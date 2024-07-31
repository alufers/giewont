#include "ClientGame.h"
#include "CameraEntity.h"
#include "Log.h"
#include "net_common.h"
#include "raylib.h"
#include "schema.capnp.h"
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
  for (auto &entity : entities) {
    if (entity != nullptr) {
      entity->draw(*this);
    }
  }

  if (debug_overlay) {
    for (auto &entity : entities) {
      if (entity != nullptr) {
        entity->draw_debug(*this);
      }
    }
  }

  camera.end_mode2d();

  DrawText(std::format("UPS: {:.2f}", last_ups).c_str(), 10, 10, 20, BLACK);
}

void ClientGame::update(float delta_time) {

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
    if (!this->camera_ref.valid(*this)) {
      LOG_WARN() << "Camera not set, creating a new one" << std::endl;
      auto camera = std::make_unique<CameraEntity>();
      this->camera_ref = this->push_entity(std::move(camera));
    }
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
