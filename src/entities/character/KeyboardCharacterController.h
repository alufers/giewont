#pragma once

#include "CharacterController.h"
#include "Game.h"
#include "Entity.h"
#include <optional>

namespace giewont {

class KeyboardCharacterController : public CharacterController {
public:
  void update(Game &game, CharacterEntity &character,
              float delta_time) override;

  
  void draw(const Game &game) override;

  private:
  uint32_t check_interactables_counter = 0;
  EntityRef highlighted_interactable;
  std::optional<AABB> highlighted_interactable_aabb = std::nullopt;
};

} // namespace giewont
