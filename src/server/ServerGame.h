#ifndef SERVERGAME_H_
#define SERVERGAME_H_

#include "Game.h"
#include "schema.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <string>

extern "C" {
#include "nbnet.h"
}

namespace giewont {

class ClientPeer;

class ServerGame : public Game {
public:
  ServerGame(std::string tmj_path);
  bool is_server() const override { return true; }

  void init_net_server();

  void update(float delta_time) override;

  EntityRef push_entity(std::unique_ptr<Entity> entity) override;

  std::vector<ClientPeer> clients;

private:
  std::string tmj_path;
  uint32_t net_id_counter = 1;
  uint32_t peer_id_counter = 1;

  void sync_entities_to_clients();

  void handle_incoming_nbnet_message(NBN_MessageInfo msg_info);

  void handle_incoming_message(ClientPeer &peer, const net::BaseNetMessage::Reader &message);

  void spawn_player_character(ClientPeer &peer);
};

class ClientPeer {
public:
  uint32_t peer_id;
  NBN_ConnectionHandle connection;
  bool level_loaded;

  void send_reliable(
      ::capnp::MallocMessageBuilder &message_builder); // TODO: can be const?
};

} // namespace giewont

#endif // SERVERGAME_H_
