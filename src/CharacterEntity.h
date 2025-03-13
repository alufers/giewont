#ifndef CHARACTERENTITY_H_
#define CHARACTERENTITY_H_

#include "AABB.h"
#include "Entity.h"
#include "Game.h"
#include "PhysEntity.h"
#include "ResourceManager.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace giewont {

class CharacterController;
class KeyboardCharacterController;

enum class CharacterMovementCommand {
  NONE = 0,
  MOVE_LEFT = 1 << 0,
  MOVE_RIGHT = 1 << 1,
  JUMP = 1 << 2,
};

enum class CharacterAnimState { STAND, WALK, JUMP };

enum class CharacterDirection { LEFT, RIGHT };

class CharacterAnimFrame {
public:
  int spritesheet_x;
  int spritesheet_y;
  int spritesheet_w;
  int spritesheet_h;
  bool flip;
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

  CharacterAnimState anim_state = CharacterAnimState::STAND;
  CharacterDirection direction = CharacterDirection::RIGHT;
  int anim_frame = 0;
  float anim_frame_duration = 0.03f;
  float anim_frame_timer = 0.0f;

  int health = 100;
  int max_health = 100;

  float min_fall_hurt_height = 200.0f;
  float max_fall_hurt_height = 500.0f;
  float fall_max_damage_factor = 0.5f; // falling can take a maximum of 50% of the health

  CharacterEntity();

  const char *get_type_name() const override { return "CharacterEntity"; }

  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  AABB &get_aabb() override { return character_aabb; }

  void perform_movement(const Game &game, float delta_time,
                        CharacterMovementCommand command);

  net::EntityType get_net_type() override;

  bool check_is_propped(TilemapEntity *tilemap, Vec2 feet_pos) override;
  void on_has_landed(Game &game, Vec2 fall_delta) override;

  /** @brief Get Feet pos in world space */
  Vec2 world_feet_pos();

  // Netcode
  void
  build_sync_message(Game &game,
                     net::SyncEntityNetMessage::Builder &sync_message) override;

  void update_from_sync_message(
      Game &game,
      const net::SyncEntityNetMessage::Reader &sync_message) override;

  /** @brief Called on the net owner of this entity to apply the effects of
   * hurting. */
  void apply_hurt_message(Game &game,
                          const net::HurtEntityNetMessage::Reader &msg);

  /** @brief Calls apply_hurt_message if entity is owned by us, otherwise
   * forwards to owner.  */
  void hurt_entity(Game &game, int damage);

private:
  res_id _texture_id;
  res_id _spritesheet_data_id;
  res_id _ui_bar_texture_id;
  res_id _character_fall_particle_system_prefab_id;
  std::unordered_map<CharacterAnimState, std::vector<CharacterAnimFrame>>
      anim_frames;
  AABB character_aabb;

  void draw_raylib_ui(const Game &game) override;

  void load_spritesheet_data(const Game &game);
};

/**
 * @brief Base class for character controllers (player, AI, remote etc.)
 */
class CharacterController {
public:
  virtual void update(Game &game, CharacterEntity &character,
                      float delta_time) = 0;
};

/**
 * @brief Character controller that does nothing, used on the client for
 * characters owned by the server.
 *
 */
class RemoteCharacterController : public CharacterController {
public:
  void update(Game &game, CharacterEntity &character,
              float delta_time) override {};
};

class KeyboardCharacterController : public CharacterController {
public:
  void update(Game &game, CharacterEntity &character,
              float delta_time) override;
};

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

} // namespace giewont
#endif // CHARACTERENTITY_H_
