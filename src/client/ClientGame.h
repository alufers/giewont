#ifndef CLIENTGAME_H_
#define CLIENTGAME_H_

#include "Entity.h"
#include "Game.h"
#include <capnp/message.h>
#include <capnp/serialize.h>

#include "schema.capnp.h"

extern "C" {
#include "nbnet.h"
}

namespace giewont {

enum class ClientGameState {
  INITIAL,
  PRE_CONNECTING, // To render one frame before connecting
  CONNECTING,
  CONNECTED,
  ERROR,
};

class ClientGame : public Game {
public:
  ClientGame(std::string server_address, int server_port);
  void draw();
  void update(float delta_time) override;
  void init_net_client();
  bool is_server() const override { return false; }
  void shutdown() override;

  ClientGameState state = ClientGameState::INITIAL;

private:
  std::string error_message;
  std::string server_address;
  int server_port;

  void handle_incoming_nbnet_message(NBN_MessageInfo msg_info);

  void handle_incoming_message(const net::BaseNetMessage::Reader &message);
  void sync_my_entities_to_server();
  void send_reliable(::capnp::MallocMessageBuilder &message_builder);

  EntityRef inspector_selected_entity;

  void draw_ui();
};
} // namespace giewont

#endif // CLIENTGAME_H_
