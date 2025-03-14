#pragma once
#include "Game.h"
namespace giewont {

class CharacterEntity; // Only forward declare here, to avoid circular includes

enum class CharacterMovementCommand {
  NONE = 0,
  MOVE_LEFT = 1 << 0,
  MOVE_RIGHT = 1 << 1,
  JUMP = 1 << 2,
};

CharacterMovementCommand operator|(CharacterMovementCommand a,
                                   CharacterMovementCommand b);

bool operator&(CharacterMovementCommand a, CharacterMovementCommand b);

CharacterMovementCommand operator|=(CharacterMovementCommand &a,
                                    CharacterMovementCommand b);

/**
 * @brief Base class for character controllers (player, AI, remote etc.)
 */
class CharacterController {
public:
  virtual void update(Game &game, CharacterEntity &character,
                      float delta_time) = 0;
};

}; // namespace giewont
