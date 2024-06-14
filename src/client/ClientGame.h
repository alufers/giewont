#ifndef CLIENTGAME_H_
#define CLIENTGAME_H_

#include "Game.h"

namespace giewont {
class ClientGame : public Game {
public:
  void draw();

  bool is_server() const override { return false; }
};
} // namespace giewont

#endif // CLIENTGAME_H_
