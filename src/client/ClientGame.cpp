#include "ClientGame.h"
#include "CameraEntity.h"
#include "Entity.h"
#include "Log.h"
#include "entities/character/CharacterEntity.h"

#include "DebugGUI.h"
#include "nbnet_helper.h"
#include "nbnet_lean.h"
#include "net_common.h"
#include "raylib.h"
#include "rlImGui.h"
#include "rlgl.h"
#include "schema.capnp.h"
#include <algorithm>
#include <cstring>
#include <format>
#include <stdexcept>
#include <stdlib.h>

#include "GameAnalytics/GameAnalytics.h"

using namespace giewont;

ClientGame::ClientGame() : DrawableGame() {

  debug_gui = std::make_unique<DebugGUI>();
}

void ClientGame::connect_to_server(const std::string &server_address,
                                   int server_port,
                                   const std::string &player_name) {
  this->server_address = server_address;
  this->server_port = server_port;
  state = ClientGameState::INITIAL;
  this->local_player_name = player_name;
}

void ClientGame::load_level(std::string tmj_path) {
  gameanalytics::GameAnalytics::addProgressionEvent(gameanalytics::EGAProgressionStatus::Complete, current_tmj_path);
  Game::load_level(tmj_path);
  gameanalytics::GameAnalytics::addProgressionEvent(gameanalytics::EGAProgressionStatus::Start, current_tmj_path);



}

void ClientGame::draw() {

  if (state != ClientGameState::CONNECTED &&
      state != ClientGameState::NO_CONNECTION_NEEDED) {
    std::string message = "Connecting to server...";
    if (state == ClientGameState::GAME_STATE_ERROR) {
      message = "Error: " + error_message;
    }

    int text_size = 20;
    int text_width = MeasureText(message.c_str(), text_size);
    DrawText(message.c_str(), GetScreenWidth() / 2 - text_width / 2,
             GetScreenHeight() / 2 - text_size / 2, text_size,
             state == ClientGameState::GAME_STATE_ERROR ? RED : BLACK);
    return;
  }

  // Todo: cache this
  std::vector<EntityRef> entity_draw_list;

  entity_draw_list.reserve(entities.size());

  for (auto &entity : entities) {
    if (entity != nullptr && entity->id != 0) {
      entity_draw_list.push_back(entity->get_ref());
    }
  }

  std::sort(entity_draw_list.begin(), entity_draw_list.end(),
            [&](const EntityRef &a, const EntityRef &b) {
              auto &ent_a = a.get_as<Entity>(*this);
              auto &ent_b = b.get_as<Entity>(*this);
              return ent_a.get_z_index() < ent_b.get_z_index();
            });

  // rendering with the camera transform
  if (camera_ref.valid_as<CameraEntity>(*this)) {

    auto &camera = camera_ref.get_as<CameraEntity>(*this);
    camera.begin_mode2d();

    for (auto &entity_ref : entity_draw_list) {
      entity_ref.get(*this).draw(*this);
    }

    debug_gui->draw(*this, entity_draw_list);

    camera.end_mode2d();
  } else {
    std::string message = "No camera set";
    int text_size = 20;
    int text_width = MeasureText(message.c_str(), text_size);
    DrawText(message.c_str(), GetScreenWidth() / 2 - text_width / 2,
             GetScreenHeight() / 2 - text_size / 2, text_size, RED);
  }

  // Rendering screen space UI

  for (auto &entity_ref : entity_draw_list) {
    entity_ref.get(*this).draw_raylib_ui(*this);
  }

  // Imgui UI drawing

  rlImGuiBegin();
  debug_gui->draw_ui(*this);
  for (auto &entity_ref : entity_draw_list) {
    entity_ref.get(*this).draw_imgui_ui(*this);
  }
  rlImGuiEnd();
}

void ClientGame::update(float delta_time) {

  if (!this->camera_ref.valid(*this)) {
    LOG_WARN() << "Camera not set, creating a new one" << std::endl;
    auto camera = std::make_unique<CameraEntity>();
    this->camera_ref = this->push_entity(std::move(camera));
  }

  if (state == ClientGameState::INITIAL) {
    state = ClientGameState::PRE_CONNECTING;
    return;
  } else if (state == ClientGameState::PRE_CONNECTING) {
    state = ClientGameState::CONNECTING;
    if (NBN_GameClient_Start(GIEWONT_PROTOCOL_NAME,
                             this->server_address.c_str(),
                             this->server_port) < 0) {
      LOG_ERROR() << "Failed to connect to server" << std::endl;
      error_message = "Failed to connect to server";
      state = ClientGameState::GAME_STATE_ERROR;
    }

    return;
  } else if (state == ClientGameState::GAME_STATE_ERROR) {
    return;
  }

  int ev;

  if (state != ClientGameState::NO_CONNECTION_NEEDED) {
    // Poll for client events
    while ((ev = NBN_GameClient_Poll()) != NBN_NO_EVENT) {
      if (ev < 0) {
        LOG_ERROR() << "An error occurred while polling for events"
                    << std::endl;

        break;
      }

      switch (ev) {
      // Client is connected to the server
      case NBN_CONNECTED:
        LOG_INFO() << "Connected to server" << std::endl;
        state = ClientGameState::CONNECTED;
        break;

        // Client has disconnected from the server
      case NBN_DISCONNECTED:
        LOG_INFO() << "Disconnected from server" << std::endl;
        break;

        // A message has been received from the server
      case NBN_MESSAGE_RECEIVED:
        NBN_MessageInfo msg_info = NBN_GameClient_GetMessageInfo();
        handle_incoming_nbnet_message(msg_info);
        break;
      }
    }
  }

  // Only update the game if the client is connected to the server
  if (state == ClientGameState::CONNECTED ||
      state == ClientGameState::NO_CONNECTION_NEEDED) {
    Game::update(delta_time);
  }

  debug_gui->update(*this);
  if (state != ClientGameState::NO_CONNECTION_NEEDED) {
    sync_my_entities_to_server();

    if (NBN_GameClient_SendPackets() < 0) {
      LOG_ERROR() << "Failed to send packets" << std::endl;
      exit(1);
    }
  }
}

void ClientGame::handle_incoming_nbnet_message(NBN_MessageInfo msg_info) {
  if (msg_info.type != NBN_BYTE_ARRAY_MESSAGE_TYPE) {
    throw std::runtime_error("Unexpected byte array message type");
  }

  NBN_ByteArrayMessage *msg = (NBN_ByteArrayMessage *)msg_info.data;
  auto received_array =
      kj::ArrayPtr<capnp::word>(reinterpret_cast<capnp::word *>(msg->bytes),
                                msg->length / sizeof(capnp::word));
  ::capnp::FlatArrayMessageReader message_receiver_builder(received_array);
  auto message = message_receiver_builder.getRoot<net::BaseNetMessage>();

  this->handle_incoming_message(message);

  NBN_ByteArrayMessage_Destroy(msg);
}

void ClientGame::handle_incoming_message(
    const net::BaseNetMessage::Reader &message) {

  switch (message.which()) {
  case net::BaseNetMessage::Which::LOAD_LEVEL: {
    LOG_INFO() << "Received load level message" << std::endl;
    load_level(message.getLoadLevel().getLevelName());
    this->my_peer_id = message.getLoadLevel().getYourPeerId();
    // Now reply
    ::capnp::MallocMessageBuilder message_builder;
    auto root = message_builder.initRoot<net::BaseNetMessage>();
    root.setLevelLoaded();
    send_reliable_to_peer(0, message_builder);
    break;
  }
  case net::BaseNetMessage::Which::SYNC_ENTITY: {

    apply_sync_entity(message.getSyncEntity());
    break;
  }

  case net::BaseNetMessage::Which::SET_CAMERA_FOLLOWED_ENTITY: {
    LOG_INFO() << "Camera followed entity set" << std::endl;
    auto &camera = camera_ref.get_as<CameraEntity>(*this);
    camera.entity_to_follow = this->get_entity_by_net_id(
        message.getSetCameraFollowedEntity().getNetId());

    LOG_INFO() << " camera.entity_to_follow = " << camera.entity_to_follow.id
               << std::endl;
    break;
  }

  case net::BaseNetMessage::Which::DESTROY_ENTITY: {
    auto net_id = message.getDestroyEntity().getNetId();
    auto entity = get_entity_by_net_id(net_id);
    if (entity.valid(*this)) {
      entity.get(*this).destroy();
    }
    break;
  }

  case net::BaseNetMessage::Which::ADD_CAMERA_EFFECT: {
    auto &camera = camera_ref.get_as<CameraEntity>(*this);
    camera.handle_add_camera_effect(message.getAddCameraEffect());
    break;
  }

  case net::BaseNetMessage::Which::HURT_ENTITY: {
    auto net_id = message.getHurtEntity().getNetId();
    auto entity = get_entity_by_net_id(net_id);
    if (entity.valid_as<CharacterEntity>(*this)) {
      auto characterEntity = &entity.get_as<CharacterEntity>(*this);

      characterEntity->apply_hurt_message(*this, message.getHurtEntity());
    }
    break;
  }
  case net::BaseNetMessage::Which::INSTANTIATE_PREFAB: {
    auto prefab_id =
        this->preload_prefab(message.getInstantiatePrefab().getPrefabPath());
    auto prefab = this->instantiate_prefab(
        prefab_id, Vec2(message.getInstantiatePrefab().getPosition()));
    break;
  }
  case net::BaseNetMessage::Which::APPLY_PHYSICS_IMPULSE: {
    auto net_id = message.getApplyPhysicsImpulse().getNetId();
    auto entity = get_entity_by_net_id(net_id);
    if (entity.valid_as<PhysEntity>(*this)) {
      auto phys_entity = &entity.get_as<PhysEntity>(*this);
      phys_entity->handle_apply_impulse_message(
          *this, message.getApplyPhysicsImpulse());
    }
    break;
  }

  default:
    LOG_WARN() << "Unknown message type received from the server" << std::endl;
  }
}

void ClientGame::send_reliable_to_peer(
    uint32_t peer_id, ::capnp::MallocMessageBuilder &message_builder) {
  if (peer_id != 0) {
    LOG_WARN() << "ClientGame::send_reliable_to_peer: peer_id != 0"
               << std::endl;
    peer_id = 0;
  }
  auto encoded_array = capnp::messageToFlatArray(message_builder);
  auto charArray = encoded_array.asChars();
  NBN_GameClient_SendReliableByteArray((unsigned char *)charArray.begin(),
                                       charArray.size());
}

void ClientGame::broadcast_reliable(
    ::capnp::MallocMessageBuilder &message_builder) {
  auto encoded_array = capnp::messageToFlatArray(message_builder);
  auto charArray = encoded_array.asChars();
  NBN_GameClient_SendReliableByteArray((unsigned char *)charArray.begin(),
                                       charArray.size());
}

void ClientGame::sync_my_entities_to_server() {

  for (auto &entity : entities) {
    if (entity != nullptr && !entity->marked_for_deletion &&
        !entity->is_static && entity->net_owner_peer_id == my_peer_id) {
      ::capnp::MallocMessageBuilder message;

      auto root = message.initRoot<net::BaseNetMessage>();

      auto syncEntities = root.initSyncEntity();

      entity->build_sync_message(*this, syncEntities);

      send_reliable_to_peer(0, message);
    }
  }
}

void ClientGame::shutdown() {
  LOG_INFO() << "Shutting down ClientGame" << std::endl;
  NBN_GameClient_Stop();
}

Camera2D ClientGame::get_currently_rendering_camera_data() const {
  if (this->camera_ref.valid(*this)) {
    auto &camera = camera_ref.get_as<CameraEntity>((Game &)*this);
    return camera.camera_after_effects;
  }
  return {0};
}

EntityRef ClientGame::spawn_player_character(uint32_t peer_id, Vec2 position) {
  throw std::runtime_error(
      "ClientGame::spawn_player_character: cannot be called on the client!");
};

ClientGame::~ClientGame() = default;

void ClientGame::perform_interaction(
    const net::InteractNetMessage::Reader &message) {

  capnp::MallocMessageBuilder root_msg;
  auto base_msg = root_msg.initRoot<net::BaseNetMessage>();
  base_msg.setInteract(message);
  this->send_reliable_to_peer(0, root_msg);
}
