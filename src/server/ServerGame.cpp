#include "ServerGame.h"
#include "Log.h"
#include "net/net_common.h"
#include "net_common.h"
#include <exception>

extern "C" {
#include "nbnet.h"
#include "net_drivers/webrtc_c.h"
}

using namespace giewont;

void ServerGame::init_net_server() {
  NBN_WebRTC_C_Register(NBN_WebRTC_C_Config{
      .enable_tls = false,
  });

  if(NBN_GameServer_Start(GIEWONT_PROTOCOL_NAME, 1338) < 0) {
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
      LOG_INFO() << "Client connected" << std::endl;
    } break;
    }
  }
  Game::update(delta_time);

  if(NBN_GameServer_SendPackets() < 0) {
    throw std::runtime_error("Failed to send packets");
  }
}
