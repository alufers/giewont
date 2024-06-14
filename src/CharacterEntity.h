#ifndef CHARACTERENTITY_H_
#define CHARACTERENTITY_H_

#include "AABB.h"
#include "Entity.h"
#include "Game.h"
#include "PhysEntity.h"
#include "ResourceManager.h"
#include "raylib.h"
#include <memory>

namespace giewont {

class CharacterController;
class KeyboardCharacterController;

enum class CharacterMovementCommand {
    NONE = 0,
    MOVE_LEFT = 1 << 0,
    MOVE_RIGHT = 1 << 1,
    JUMP = 1 << 2,
};



/**
 * @brief Entiity for player and non-player characters.
 */
class CharacterEntity : public PhysEntity {
public:
  std::unique_ptr<CharacterController> controller;

  float max_horiz_speed = 300.0f;
  float horiz_accel = 2000.0f;
  float jump_speed = 500.0f;

  CharacterEntity();
  void load_assets(const Game &game) override;
  void update(const Game &game, float delta_time) override;
  void draw(const Game &game) override;

  AABB &get_aabb() override { return character_aabb; }

  void perform_movement(const Game &game, float delta_time, CharacterMovementCommand command);

private:
  res_id _texture_id;
  AABB character_aabb;
};

/**
 * @brief Base class for character controllers (player, AI, remote etc.)
 */
class CharacterController {
public:
  virtual void update(const Game &game, CharacterEntity &character,
                      float delta_time) = 0;
};

class KeyboardCharacterController : public CharacterController {
public:
  void update(const Game &game, CharacterEntity &character,
              float delta_time) override;
};

} // namespace giewont
#endif // CHARACTERENTITY_H_
