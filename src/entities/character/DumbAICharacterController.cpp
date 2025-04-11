#include "DumbAICharacterController.h"
#include "math/RandUtil.h"

using namespace giewont;

void DumbAICharacterController::update(Game &game, CharacterEntity &character,
                                       float delta_time) {
  CharacterMovementCommand command = CharacterMovementCommand::NONE;
  time_since_last_jump += delta_time;
  if (dir_change_time > 0.0f) {
    dir_change_time -= delta_time;
  } else if (dir_change_time > -1000.0f) {
    dir_change_time = -10000.0f;
    moving_right = !moving_right;
    dwell_time = rand_float(0.5f, 2.0f);
  }

  if (dwell_time > 0.0) {
    dwell_time -= delta_time;
  } else {
    Vec2 feet_pos =
        character.position +
        Vec2((character.get_aabb().min.x + character.get_aabb().max.x) / 2.0f,
             character.get_aabb().max.y + 1.0f);
    Vec2 head_pos =
        character.position +
        Vec2((character.get_aabb().min.x + character.get_aabb().max.x) / 2.0f,
             (character.get_aabb().max.y + character.get_aabb().min.y) / 2.0f +
                 1.0f);

    for (auto &entity : game.valid_entities()) {
      if (TilemapEntity *tilemap =
              dynamic_cast<TilemapEntity *>(entity.get())) {
        Vec2 pos_to_check = feet_pos;
        pos_to_check.x +=
            ((character.get_aabb().min.x + character.get_aabb().max.x) / 2.0f +
             tilemap->tile_size.x / 2.0f) *
            (moving_right ? 1.0f : -1.0f);

        Vec2 head_pos_to_check = head_pos;
        head_pos_to_check.x +=
            ((character.get_aabb().min.x + character.get_aabb().max.x) / 2.0f +
             tilemap->tile_size.x / 2.0f) *
            (moving_right ? 1.0f : -1.0f);
        if (!character.check_is_propped(tilemap, pos_to_check) &&
            dir_change_time <= 0.0f) {

          dwell_time = 1.0f;
          moving_right = !moving_right;
          break;
        }

        if (character.check_is_propped(tilemap, head_pos_to_check) &&
            dir_change_time <= 0.0f && time_since_last_jump > 7.0f) {
          time_since_last_jump = 0.0f;
          command |= CharacterMovementCommand::JUMP;
          // moving_right = !moving_right;
          dir_change_time = rand_float(1.5f, 3.0f);
        }
      }
    }

    if (moving_right) {
      command |= CharacterMovementCommand::MOVE_RIGHT;
    } else {
      command |= CharacterMovementCommand::MOVE_LEFT;
    }
  }

  character.perform_movement(game, delta_time, command);
}
