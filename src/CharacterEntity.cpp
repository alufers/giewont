#include "CharacterEntity.h"
#include "AABB.h"
#include "Game.h"
#include "PhysEntity.h"
#include "TilemapEntity.h"
#include <cmath>

#ifdef GIEWONT_HAS_GRAPHICS
#include <raylib.h>
#endif

using namespace giewont;

CharacterMovementCommand operator|(CharacterMovementCommand a,
                                   CharacterMovementCommand b) {
  return static_cast<CharacterMovementCommand>(static_cast<int>(a) |
                                               static_cast<int>(b));
}

bool operator&(CharacterMovementCommand a, CharacterMovementCommand b) {
  return static_cast<int>(a) & static_cast<int>(b);
}

CharacterMovementCommand operator|=(CharacterMovementCommand &a,
                                    CharacterMovementCommand b) {
  a = static_cast<CharacterMovementCommand>(static_cast<int>(a) |
                                            static_cast<int>(b));
  return a;
}

CharacterEntity::CharacterEntity() : PhysEntity() {
  this->controller = std::make_unique<KeyboardCharacterController>();
}

void CharacterEntity::load_assets(const Game &game) {

  _texture_id = game.rm->load_texture("entites/slime.png");
#ifdef GIEWONT_HAS_GRAPHICS
  // TODO: remove this, can't load texture on the server
  auto tex = game.rm->get_texture(_texture_id);
  character_aabb =
      AABB::from_min_and_size(Vec2(0, 0), Vec2(tex->width, tex->height));
#endif
}

void CharacterEntity::update(Game &game, float delta_time) {
  this->controller->update(game, *this, delta_time);
  PhysEntity::update(game, delta_time);
}

void CharacterEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_id);
  DrawTexture(*tex, position.x, position.y, WHITE);
#endif
}

void CharacterEntity::perform_movement(const Game &game, float delta_time,
                                       CharacterMovementCommand command) {
  bool on_ground_or_ladder = false;

  Vec2 feet_pos = this->position +
                  Vec2((this->get_aabb().min.x + this->get_aabb().max.x) / 2.0f,
                       this->get_aabb().max.y + 1.0f);
  for (auto &entity : game.entities) {
    if (entity->id == this->id || entity == nullptr ||
        entity->marked_for_deletion) {
      continue;
    }

    if (TilemapEntity *tilemap = dynamic_cast<TilemapEntity *>(entity.get())) {
      if (tilemap->check_allow_jump(feet_pos)) {
        on_ground_or_ladder = true;
        break;
      }
    }
  }

  if (command & CharacterMovementCommand::JUMP && on_ground_or_ladder) {
    this->velocity.y = -this->jump_speed;
  }

  if (command & CharacterMovementCommand::MOVE_LEFT) {
    this->velocity.x -= this->horiz_accel * delta_time;
    if (this->velocity.x < -this->max_horiz_speed) {
      this->velocity.x = -this->max_horiz_speed;
    }
  } else if (command & CharacterMovementCommand::MOVE_RIGHT) {
    this->velocity.x += this->horiz_accel * delta_time;
    if (this->velocity.x > this->max_horiz_speed) {
      this->velocity.x = this->max_horiz_speed;
    }
  } else {
    if (on_ground_or_ladder) {
      if (std::fabs(this->velocity.x) > PHYS_EPSILON) {
        float sign = this->velocity.x > 0 ? 1.0f : -1.0f;
        this->velocity.x -= sign * this->horiz_accel * delta_time;
        if (sign * this->velocity.x < 0) {
          this->velocity.x = 0;
        }
      }
    }
  }
}

void KeyboardCharacterController::update(Game &game, CharacterEntity &character,
                                         float delta_time) {

#ifdef GIEWONT_HAS_GRAPHICS
  CharacterMovementCommand command = CharacterMovementCommand::NONE;
  if (IsKeyDown(KEY_A)) {
    command |= CharacterMovementCommand::MOVE_LEFT;
  } else if (IsKeyDown(KEY_D)) {
    command |= CharacterMovementCommand::MOVE_RIGHT;
  }
  if (IsKeyDown(KEY_SPACE)) {
    command |= CharacterMovementCommand::JUMP;
  }

  character.perform_movement(game, delta_time, command);
#endif
}

void DumbAICharacterController::update(Game &game, CharacterEntity &character,
                                       float delta_time) {

  CharacterMovementCommand command = CharacterMovementCommand::NONE;
  if (dwell_time > 0.0) {
    dwell_time -= delta_time;
  } else {
    Vec2 feet_pos =
        character.position +
        Vec2((character.get_aabb().min.x + character.get_aabb().max.x) / 2.0f,
             character.get_aabb().max.y + 1.0f);

    for (auto &entity : game.entities) {
      if (entity->id == character.id || entity == nullptr ||
          entity->marked_for_deletion) {
        continue;
      }

      if (TilemapEntity *tilemap =
              dynamic_cast<TilemapEntity *>(entity.get())) {
        Vec2 pos_to_check = feet_pos;
        pos_to_check.x +=
            ((character.get_aabb().min.x + character.get_aabb().max.x) / 2.0f +
             tilemap->tile_size.x / 2.0f) *
            (moving_right ? 1.0f : -1.0f);

        if (!tilemap->check_allow_jump(pos_to_check)) {

          dwell_time = 1.0f;
          moving_right = !moving_right;
          break;
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
