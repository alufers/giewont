#pragma once

#include "CharacterController.h"
#include "Game.h"

namespace giewont {

class KeyboardCharacterController : public CharacterController {
public:
  void update(Game &game, CharacterEntity &character,
              float delta_time) override;
};

} // namespace giewont
