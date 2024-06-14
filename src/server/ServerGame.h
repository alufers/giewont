#ifndef SERVERGAME_H_
#define SERVERGAME_H_

#include "Game.h"
#include "NetBuffer.h"
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

  std::vector<ClientPeer> clients;

private:
  std::string tmj_path;
};

class ClientPeer {
public:
  NBN_ConnectionHandle connection;
  bool level_loaded;

  void send_reliable(const NetBuffer &buffer);
};

} // namespace giewont

#endif // SERVERGAME_H_
