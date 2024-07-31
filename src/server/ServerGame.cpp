#include "ServerGame.h"
#include "CameraEntity.h"
#include "CharacterEntity.h"
#include "Entity.h"
#include "Log.h"
#include "SpawnEntity.h"
#include "net/net_common.h"
#include "net_common.h"
#include "schema.capnp.h"
#include <exception>
#include <memory>

extern "C" {
#include "nbnet.h"
#include "net_drivers/webrtc_c.h"
}

using namespace giewont;

ServerGame::ServerGame(std::string tmj_path) : Game(), tmj_path(tmj_path) {
  load_level(tmj_path);
}

void ServerGame::init_net_server() {
  NBN_WebRTC_C_Register(NBN_WebRTC_C_Config{
      .enable_tls = false,
  });

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

      this->spawn_player_character(this->clients[this->clients.size() - 1]);
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

  for (auto &entity : entities) {
    if (entity != nullptr && !entity->marked_for_deletion &&
        !entity->is_static) {
    }
  }
  return ref;
}

void ServerGame::sync_entities_to_clients() {

  for (auto &entity : entities) {
    if (entity != nullptr && !entity->marked_for_deletion &&
        !entity->is_static) {
      ::capnp::MallocMessageBuilder message;

      auto root = message.initRoot<net::BaseNetMessage>();

      auto syncEntities = root.initSyncEntity();

      entity->build_sync_message(syncEntities);

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
  default:
    LOG_WARN() << "Unknown message type received from the client" << std::endl;
  }
}

void ServerGame::spawn_player_character(ClientPeer &peer) {
  EntityRef spawn_point;

  for (auto &entity : this->entities) {
    if (entity == nullptr || entity->marked_for_deletion) {
      continue;
    }

    if (SpawnEntity *spawnEnt = dynamic_cast<SpawnEntity *>(entity.get())) {
      if (spawnEnt->entity_type == "player_character") {
        spawn_point = entity->get_ref();
        break;
      }
    }
  }

  if (!spawn_point.valid(*this)) {
    LOG_WARN() << "No spawn point found for player character" << std::endl;
    return;
  }

  std::unique_ptr<CharacterEntity> character =
      std::make_unique<CharacterEntity>();
  character->controller = std::make_unique<RemoteCharacterController>();

  character->position = spawn_point.get(*this).position;
  character->net_owner_peer_id = peer.peer_id;
  character->load_assets(*this);

  EntityRef ref = this->push_entity(std::move(character));

  peer.camera_target_net_id = ref.get(*this).net_id;
  peer.set_camera_target_countdown = 30;
}

void ClientPeer::send_reliable(capnp::MallocMessageBuilder &message_builder) {
  auto encoded_array = capnp::messageToFlatArray(message_builder);
  auto charArray = encoded_array.asChars();
  NBN_GameServer_SendReliableByteArrayTo(
      connection, (unsigned char *)charArray.begin(), charArray.size());
}
