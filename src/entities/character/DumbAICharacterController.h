#pragma once

#include "CharacterEntity.h"
#include "CharacterController.h"

namespace giewont {

class DumbAICharacterController : public CharacterController {
public:
  void update(Game &game, CharacterEntity &character,
              float delta_time) override;

private:
  float dwell_time = 0.0;
  float dir_change_time = -10000.0f;
  float time_since_last_jump = 0.0f;
  bool moving_right = false;
};

}; // namespace giewont
