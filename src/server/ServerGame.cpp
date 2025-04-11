#include "ServerGame.h"
#include "CameraEntity.h"
#include "entities/character/CharacterEntity.h"
#include "entities/character/DumbAICharacterController.h"
#include "Entity.h"
#include "GrenadeEntity.h"
#include "Log.h"
#include "SpawnEntity.h"
#include "entities/gui/TeamChoiceGUIEntity.h"
#include "nbnet_helper.h"
#include "net/net_common.h"
#include "net_common.h"
#include "schema.capnp.h"
#include <algorithm>
#include <exception>
#include <memory>
#include <vector>

extern "C" {
#include "nbnet.h"
}

using namespace giewont;

ServerGame::ServerGame(std::string tmj_path) : Game(), tmj_path(tmj_path) {
  load_level(tmj_path);
}

void ServerGame::init_net_server() {
  install_nbnet_webrtc_driver();
  if (NBN_GameServer_Start(GIEWONT_PROTOCOL_NAME, 1338) < 0) {
    throw std::runtime_error("Failed to start server");
  }
}

void ServerGame::update(float delta_time) {

  // Receive all network events first
  int ev;
  while ((ev = NBN_GameServer_Poll()) != NBN_NO_EVENT) {
    switch (ev) {
    case NBN_NEW_CONNECTION: {
      NBN_GameServer_AcceptIncomingConnection();
      NBN_ConnectionHandle client = NBN_GameServer_GetIncomingConnection();
      ClientPeer peer = {
          .peer_id = peer_id_counter++,
          .connection = client,
          .level_loaded = false,
      };
      LOG_INFO() << "Client connected, peer_id " << peer.peer_id << std::endl;

      peer.connection = client;

      ::capnp::MallocMessageBuilder message;

      auto root = message.initRoot<net::BaseNetMessage>();

      auto initLoadLevel = root.initLoadLevel();
      initLoadLevel.setLevelName(tmj_path);
      initLoadLevel.setYourPeerId(peer.peer_id);
      peer.send_reliable(message);
      clients.push_back(peer);

      this->show_team_choice_gui(this->clients[this->clients.size() - 1]);
    } break;
    case NBN_CLIENT_DISCONNECTED: {
      LOG_INFO() << "Client disconnected" << std::endl;
      for (auto it = clients.begin(); it != clients.end(); it++) {
        if (it->connection == NBN_GameServer_GetDisconnectedClient()) {

          clients.erase(it);
          break;
        }
      }
    } break;
    case NBN_CLIENT_MESSAGE_RECEIVED:
      handle_incoming_nbnet_message(NBN_GameServer_GetMessageInfo());
      break;
    }
  }
  Game::update(delta_time);

  this->sync_entities_to_clients();

  if (NBN_GameServer_SendPackets() < 0) {
    throw std::runtime_error("Failed to send packets");
  }
}

EntityRef ServerGame::push_entity(std::unique_ptr<Entity> entity) {
  // call super method
  EntityRef ref = Game::push_entity(std::move(entity));

  ref.get_as<Entity>(*this).net_id = this->net_id_counter++;

  return ref;
}

void ServerGame::sync_entities_to_clients() {

  for (auto &entity : entities) {
    if (entity != nullptr && !entity->marked_for_deletion &&
        !entity->is_static) {
      if (entity->net_id == 0) {
        LOG_ERROR() << "Trying to sync entity with net_id 0 ("
                    << entity->get_type_name() << ", id: " << entity->id << ")"
                    << std::endl;
        throw std::runtime_error("Trying to sync entity with net_id 0");
      }
      ::capnp::MallocMessageBuilder message;

      auto root = message.initRoot<net::BaseNetMessage>();

      auto syncEntities = root.initSyncEntity();

      entity->build_sync_message(*this, syncEntities);

      for (auto &client : clients) {
        // Sync only to clients which have loaded the level

        if (client.level_loaded) {
          client.send_reliable(message);
        }
      }
    }
  }

  for (auto &client : clients) {
    if (client.set_camera_target_countdown >= 0) {
      client.set_camera_target_countdown--;

      if (client.set_camera_target_countdown == 0) {
        ::capnp::MallocMessageBuilder message;

        auto root = message.initRoot<net::BaseNetMessage>();

        auto setCameraFollowedEntity = root.initSetCameraFollowedEntity();
        setCameraFollowedEntity.setNetId(client.camera_target_net_id);

        client.send_reliable(message);
      }
    }
  }
}

void ServerGame::handle_incoming_nbnet_message(NBN_MessageInfo msg_info) {
  ClientPeer *peer = nullptr;

  for (auto &client : clients) {
    if (client.connection == msg_info.sender) {
      peer = &client;
    }
  }

  if (peer == nullptr) {
    LOG_WARN() << "Received message from unknown peer" << std::endl;
    return;
  }

  if (msg_info.type != NBN_BYTE_ARRAY_MESSAGE_TYPE) {
    throw std::runtime_error("Unexpected byte array message type");
  }

  NBN_ByteArrayMessage *msg = (NBN_ByteArrayMessage *)msg_info.data;

  auto received_array =
      kj::ArrayPtr<capnp::word>(reinterpret_cast<capnp::word *>(msg->bytes),
                                msg->length / sizeof(capnp::word));
  ::capnp::FlatArrayMessageReader message_receiver_builder(received_array);
  auto message = message_receiver_builder.getRoot<net::BaseNetMessage>();
  this->handle_incoming_message(*peer, message);
}

void ServerGame::handle_incoming_message(
    ClientPeer &peer, const net::BaseNetMessage::Reader &message) {
  switch (message.which()) {
  case net::BaseNetMessage::Which::LEVEL_LOADED: {
    LOG_INFO() << "Client with peer_id " << peer.peer_id << " loaded level"
               << std::endl;
    peer.level_loaded = true;
  } break;

  case net::BaseNetMessage::Which::SYNC_ENTITY: {
    apply_sync_entity(message.getSyncEntity());
  } break;

  case net::BaseNetMessage::Which::INTERACT: {
    handle_interact_message(peer, message.getInteract());
  } break;

  case net::BaseNetMessage::Which::GUI_INTERACTION: {
    handle_gui_interaction_message(peer, message.getGuiInteraction());
  } break;
  case net::BaseNetMessage::Which::DESTROY_ENTITY: {
    handle_destroy_entity_message(peer, message.getDestroyEntity());
    break;
  }
  default:
    LOG_WARN() << "Unknown message type received from the client "
               << static_cast<int>(message.which()) << std::endl;
  }
}

EntityRef ServerGame::spawn_player_character(uint32_t peer_id, Vec2 position) {

  if (peer_id == 0) {
    // Spawn AI character
    auto character = std::make_unique<CharacterEntity>();
    character->nickname = "[AI]";
    character->controller = std::make_unique<DumbAICharacterController>();
    character->position = position;
    character->net_owner_peer_id = peer_id;
    character->load_assets(*this);
    EntityRef ref = this->push_entity(std::move(character));
    return ref;
  }

  for (auto &client : clients) {
    if (client.peer_id == peer_id) {
      std::unique_ptr<CharacterEntity> character =
          std::make_unique<CharacterEntity>();
      character->controller = std::make_unique<RemoteCharacterController>();

      character->position = position;
      character->net_owner_peer_id = peer_id;
      character->load_assets(*this);

      EntityRef ref = this->push_entity(std::move(character));

      client.camera_target_net_id = ref.get(*this).net_id;
      client.set_camera_target_countdown = 10;

      return ref;
    }
  }

  LOG_WARN() << "spawn_player_character: peer_id " << peer_id
             << " not found in clients" << std::endl;
  return EntityRef();
}

void ServerGame::show_team_choice_gui(ClientPeer &peer) {
  std::unique_ptr<TeamChoiceGUIEntity> choice_gui =
      std::make_unique<TeamChoiceGUIEntity>();

  choice_gui->position = Vec2(0, 0);
  choice_gui->net_owner_peer_id = peer.peer_id;
  choice_gui->load_assets(*this);

  EntityRef ref = this->push_entity(std::move(choice_gui));
}

void ServerGame::send_reliable_to_peer(
    uint32_t peer_id, capnp::MallocMessageBuilder &message_builder) {
  for (auto &client : clients) {
    if (client.peer_id == peer_id) {
      client.send_reliable(message_builder);
      return;
    }
  }
  LOG_WARN() << "send_reliable_to_peer: peer_id " << peer_id
             << " not found in clients" << std::endl;
}

void ServerGame::handle_interact_message(
    ClientPeer &peer, const net::InteractNetMessage::Reader &message) {
  EntityRef interactor =
      this->get_entity_by_net_id(message.getInteractorNetId());

  if (!interactor.valid(*this)) {
    LOG_WARN() << "handle_interact_message: Interactor entity not found"
               << std::endl;
    return;
  }
  auto &interactor_ent = interactor.get(*this);

  if (interactor_ent.net_owner_peer_id != peer.peer_id) {
    LOG_WARN() << "handle_interact_message: Interactor entity does not belong "
                  "to the peer"
               << std::endl;
    return;
  }

  Vec2 interactor_pos = interactor_ent.position;
  if (message.getType() == net::InteractionType::USE_INTERACTION) {

    std::vector<EntityRef> allEntities;
    for (auto &entity : this->valid_entities()) {
      if (entity->get_ref() == interactor)
        continue;

      allEntities.push_back(entity->get_ref());
    }

    std::sort(allEntities.begin(), allEntities.end(),
              [&](EntityRef a, EntityRef b) {
                return a.get(*this).position.distance(interactor_pos) <
                       b.get(*this).position.distance(interactor_pos);
              });

    for (auto &entity_ref : allEntities) {
      auto &entity = entity_ref.get(*this);
      if (entity.handle_interaction(*this, message))
        break;
    }
  } else if (message.getType() == net::InteractionType::THROW_INTERACTION) {
    auto grenade = std::make_unique<GrenadeEntity>();
    grenade->position = interactor_pos;
    if (interactor.valid_as<PhysEntity>(*this)) {
      grenade->velocity = interactor.get_as<PhysEntity>(*this).velocity * 1.2;
    }
    grenade->load_assets(*this);
    push_entity(std::move(grenade));
  }
}

void ServerGame::handle_gui_interaction_message(
    ClientPeer &peer, const net::GuiInteractionNetMessage::Reader &message) {
  EntityRef interactor = this->get_entity_by_net_id(message.getNetId());

  if (!interactor.valid(*this)) {
    LOG_WARN() << "handle_gui_interaction_message: GUI entity not found"
               << std::endl;
    return;
  }
  auto &interactor_ent = interactor.get(*this);

  if (interactor_ent.net_owner_peer_id != peer.peer_id) {
    LOG_WARN() << "handle_gui_interaction_message: GUI entity does not belong "
                  "to the peer"
               << std::endl;
    return;
  }

  interactor_ent.handle_gui_interaction(*this, message);
}

void ServerGame::handle_destroy_entity_message(
    ClientPeer &peer, const net::DestroyEntityNetMessage::Reader &message) {
  EntityRef entRef = this->get_entity_by_net_id(message.getNetId());

  if (!entRef.valid(*this)) {
    LOG_WARN() << "handle_destroy_entity_message: entity to destroy not found"
               << std::endl;
    return;
  }
  auto &ent = entRef.get(*this);

  if (ent.net_owner_peer_id != peer.peer_id) {
    LOG_WARN()
        << "handle_destroy_entity_message: entity does not belong to the peer"
        << std::endl;
    return;
  }

  ent.destroy();
}

void ServerGame::broadcast_reliable(
    capnp::MallocMessageBuilder &message_builder) {
  for (auto &client : clients) {
    if (client.level_loaded) {
      client.send_reliable(message_builder);
    }
  }
}

void ClientPeer::send_reliable(capnp::MallocMessageBuilder &message_builder) {
  auto encoded_array = capnp::messageToFlatArray(message_builder);
  auto charArray = encoded_array.asChars();
  NBN_GameServer_SendReliableByteArrayTo(
      connection, (unsigned char *)charArray.begin(), charArray.size());
}
