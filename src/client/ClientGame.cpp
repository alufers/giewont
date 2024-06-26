#include "ClientGame.h"
#include "CameraEntity.h"
#include "Log.h"
#include "net_common.h"
#include "raylib.h"
#include <format>
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
      LOG_INFO() << "Message received from server" << std::endl;
      break;
    }
  }

  // Only update the game if the client is connected to the server
  if (state == ClientGameState::CONNECTED) {
    Game::update(delta_time);
  }

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
