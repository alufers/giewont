#ifndef CLIENTGAME_H_
#define CLIENTGAME_H_
#define WIN32_LEAN_AND_MEAN

#include "DrawableGame.h"
#include "Entity.h"
#include "Game.h"
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <memory>

#include "schema.capnp.h"

#include "nbnet_lean.h"

namespace giewont {

enum class ClientGameState {
  NO_CONNECTION_NEEDED,
  INITIAL,
  PRE_CONNECTING, // To render one frame before connecting
  CONNECTING,
  CONNECTED,
  GAME_STATE_ERROR,
};

class DebugGUI;

class ClientGame : public DrawableGame {
public:
  ClientGame();
  void draw() override;
  void update(float delta_time) override;
  bool is_server() const override { return false; }
  void shutdown() override;

  void connect_to_server(const std::string &server_address, int server_port,
                         const std::string &player_name);

  Camera2D get_currently_rendering_camera_data() const override;

  ClientGameState state = ClientGameState::NO_CONNECTION_NEEDED;

  std::unique_ptr<DebugGUI> debug_gui;

  void send_reliable_to_peer(
      uint32_t peer_id,
      ::capnp::MallocMessageBuilder &message_builder) override;

  void
  broadcast_reliable(::capnp::MallocMessageBuilder &message_builder) override;

  EntityRef spawn_player_character(uint32_t peer_id, Vec2 position) override;

  void
  perform_interaction(const net::InteractNetMessage::Reader &message) override;

  ~ClientGame();

private:
  std::string error_message;
  std::string server_address;
  int server_port;

  void handle_incoming_nbnet_message(NBN_MessageInfo msg_info);

  void handle_incoming_message(const net::BaseNetMessage::Reader &message);
  void sync_my_entities_to_server();

  EntityRef inspector_selected_entity;
};
} // namespace giewont

#endif // CLIENTGAME_H_
