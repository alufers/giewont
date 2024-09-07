#include "ClientGame.h"
#include "CameraEntity.h"
#include "Entity.h"
#include "Log.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
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
extern "C" {
#include "nbnet.h"
#ifdef PLATFORM_WEB
#include "net_drivers/webrtc.h"
#else
#include "net_drivers/webrtc_c.h"
#endif
}

using namespace giewont;

ClientGame::ClientGame(std::string server_address, int server_port) : Game() {
  this->server_address = server_address;
  this->server_port = server_port;
}

void ClientGame::draw() {

  if (state != ClientGameState::CONNECTED) {
    std::string message = "Connecting to server...";
    if (state == ClientGameState::ERROR) {
      message = "Error: " + error_message;
    }

    int text_size = 20;
    int text_width = MeasureText(message.c_str(), text_size);
    DrawText(message.c_str(), GetScreenWidth() / 2 - text_width / 2,
             GetScreenHeight() / 2 - text_size / 2, text_size,
             state == ClientGameState::ERROR ? RED : BLACK);
    return;
  }
  auto &camera = camera_ref.get_as<CameraEntity>(*this);
  camera.begin_mode2d();

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

  for (auto &entity_ref : entity_draw_list) {
    entity_ref.get(*this).draw(*this);
  }

  if (debug_overlay) {
    for (auto &entity_ref : entity_draw_list) {
      entity_ref.get(*this).draw_debug(*this);
    }
  }

  if (inspector_selected_entity.valid(*this)) {

    auto &ent = inspector_selected_entity.get_as<Entity>(*this);
    DrawCircleV(ent.position.to_raylib(), 25.0f, RED);
  }

  camera.end_mode2d();

  DrawText(std::format("UPS: {:.2f}", last_ups).c_str(), 10, 10, 20, BLACK);

  draw_ui();
}

void ClientGame::draw_ui() {
  rlImGuiBegin();
  if (debug_ui) {

    ImGui::Begin("Entities", &debug_ui, 0);

    std::string filter = "";
    ImGui::InputTextWithHint("Filter", "Filter", &filter);

    if (ImGui::BeginTable("entities_table", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
                          ImVec2(0, 320))) {
      ImGui::TableSetupColumn("Local ID");
      ImGui::TableSetupColumn("Net ID");
      ImGui::TableSetupColumn("Type name");
      ImGui::TableSetupColumn("Flags");
      ImGui::TableHeadersRow();
      for (size_t idx = 0; idx < entities.size(); idx++) {
        auto &ent = entities[idx];
        if (ent != nullptr) {
          if (!filter.empty() &&
              strcasestr(ent->get_type_name(), filter.c_str()) == nullptr) {
            continue;
          }
          ImGui::TableNextRow();
          if (ent->is_static) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5, 0.5, 0.5, 1.0));
          }
          ImGui::TableNextColumn();
          bool is_row_selected =
              this->inspector_selected_entity == ent->get_ref();
          char label[256];
          snprintf(label, sizeof(label), "%zu", idx);
          ImGui::Selectable(label, &is_row_selected,
                            ImGuiSelectableFlags_SpanAllColumns);
          if (is_row_selected) {
            this->inspector_selected_entity = ent->get_ref();
          }
          ImGui::TableNextColumn();
          ImGui::Text("%u", ent->net_id);
          ImGui::TableNextColumn();
          ImGui::Text("%s", ent->get_type_name());
          ImGui::TableNextColumn();
          ImGui::Text("%s", ent->is_static ? "static" : "");
          if (ent->is_static) {
            ImGui::PopStyleColor();
          }
        }
      }
      ImGui::EndTable();
    }

    if (inspector_selected_entity.valid(*this)) {
      auto &ent = inspector_selected_entity.get_as<Entity>(*this);
      ImGui::BeginChild("Inspector", ImVec2(0, 0), true);
      ent.draw_inspector_ui(*this);
      ImGui::EndChild();
    }

    ImGui::End();
  }

  rlImGuiEnd();
}

void ClientGame::update(float delta_time) {

  // Check global keybinds
  if (IsKeyReleased(KEY_F11)) {
    debug_ui = !debug_ui;
  }

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
      state = ClientGameState::ERROR;
    }

    return;
  } else if (state == ClientGameState::ERROR) {
    return;
  }

  int ev;

  // Poll for client events
  while ((ev = NBN_GameClient_Poll()) != NBN_NO_EVENT) {
    if (ev < 0) {
      LOG_ERROR() << "An error occurred while polling for events" << std::endl;

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

  // Only update the game if the client is connected to the server
  if (state == ClientGameState::CONNECTED) {
    Game::update(delta_time);
  }

  sync_my_entities_to_server();

  if (NBN_GameClient_SendPackets() < 0) {
    LOG_ERROR() << "Failed to send packets" << std::endl;
    exit(1);
  }
}

void ClientGame::init_net_client() {
  NBN_WebRTC_C_Register(NBN_WebRTC_C_Config{
      .enable_tls = false,
  });
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
    send_reliable(message_builder);
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

  default:
    LOG_WARN() << "Unknown message type received from the server" << std::endl;
  }
}

void ClientGame::send_reliable(::capnp::MallocMessageBuilder &message_builder) {
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

      entity->build_sync_message(syncEntities);

      send_reliable(message);
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
