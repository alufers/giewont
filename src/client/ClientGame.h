#ifndef CLIENTGAME_H_
#define CLIENTGAME_H_

#include "Game.h"

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

  ClientGameState state = ClientGameState::INITIAL;

private:
  std::string error_message;
  std::string server_address;
  int server_port;
};
} // namespace giewont

#endif // CLIENTGAME_H_
