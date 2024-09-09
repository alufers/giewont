#ifndef CLIENTGAME_H_
#define CLIENTGAME_H_

#include "Entity.h"
#include "Game.h"
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <memory>

#include "schema.capnp.h"

extern "C" {
#include "nbnet.h"
}

namespace giewont {

enum class ClientGameState {
  INITIAL = 0,
  PRE_CONNECTING = 1, // To render one frame before connecting
  CONNECTING = 2,
  CONNECTED = 3,
  ERROR = 4
};

class DebugGUI;

class ClientGame : public Game {
public:
  ClientGame(std::string server_address, int server_port);
  void draw();
  void update(float delta_time) override;
  void init_net_client();
  bool is_server() const override { return false; }
  void shutdown() override;

  Camera2D get_currently_rendering_camera_data() const override;

  ClientGameState state = ClientGameState::INITIAL;

  std::unique_ptr<DebugGUI> debug_gui;

  void send_reliable_to_peer(
      uint32_t peer_id,
      ::capnp::MallocMessageBuilder &message_builder) override;

  void
  broadcast_reliable(::capnp::MallocMessageBuilder &message_builder) override;

  ~ClientGame();

private:
  std::string error_message;
  std::string server_address;
  int server_port;

  void handle_incoming_nbnet_message(NBN_MessageInfo msg_info);

  void handle_incoming_message(const net::BaseNetMessage::Reader &message);
  void sync_my_entities_to_server();

  EntityRef inspector_selected_entity;

  void draw_ui();
};
} // namespace giewont

#endif // CLIENTGAME_H_
