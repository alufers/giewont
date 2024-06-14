#ifndef SERVERGAME_H_
#define SERVERGAME_H_

#include "Game.h"

extern "C" {
#include "nbnet.h"

}

namespace giewont {

class ClientPeer;


class ServerGame : public Game {
public:
  bool is_server() const override { return true; }

  void init_net_server();

  void update(float delta_time) override;

  std::vector<ClientPeer> clients;
};


class ClientPeer {
  NBN_ConnectionHandle connection;
  bool level_loaded;
};

} // namespace giewont

#endif // SERVERGAME_H_
