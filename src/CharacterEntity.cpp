#include "CharacterEntity.h"
#include "AABB.h"

#include "FlagEntity.h"
#include "Game.h"
#include "Log.h"
#include "PhysEntity.h"
#include "TilemapEntity.h"
#include "schema.capnp.h"
#include <capnp/message.h>
#include <cmath>
#include <cstring>
#include <nlohmann/json.hpp>
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
  this->controller = std::make_unique<
      RemoteCharacterController>(); // Set to remote by default, the server will
                                    // change it when spawning anyway
}

void CharacterEntity::load_assets(const Game &game) {

  if (!game.is_server() && this->net_owner_peer_id == game.my_peer_id) {
    this->controller = std::make_unique<KeyboardCharacterController>();
  }

  _texture_id = game.rm->load_texture("entities/p1_spritesheet.png");
  _spritesheet_data_id = game.rm->load_json("entities/p1_spritesheet.json");

  load_spritesheet_data(game);
}

void CharacterEntity::build_sync_message(
    Game &game, net::SyncEntityNetMessage::Builder &sync_message) {
  PhysEntity::build_sync_message(game, sync_message);
  auto characterData = sync_message.initExtraData().initCharacterData();

  characterData.setAnimationState(static_cast<uint32_t>(anim_state));
  characterData.setDirection(static_cast<uint32_t>(direction));
}

void CharacterEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  PhysEntity::update_from_sync_message(game, sync_message);
  auto characterData = sync_message.getExtraData().getCharacterData();

  anim_state =
      static_cast<CharacterAnimState>(characterData.getAnimationState());
  direction = static_cast<CharacterDirection>(characterData.getDirection());
}

void CharacterEntity::load_spritesheet_data(const Game &game) {
  auto data = game.rm->get_json(_spritesheet_data_id);

  // check if it is a dictionary
  if (!data->is_object()) {
    throw std::runtime_error("Spritesheet data is not a dictionary");
  }

  for (auto &[key, value] : data->items()) {
    if (!value.is_array() || value.size() != 4) {
      throw std::runtime_error(
          "Spritesheet data is not an array of four elements (x, y, w, h)");
    }

    CharacterAnimState state = CharacterAnimState::STAND;
    if (strcasestr(key.c_str(), "stand")) {
      state = CharacterAnimState::STAND;
    } else if (strcasestr(key.c_str(), "walk")) {
      state = CharacterAnimState::WALK;
    } else if (strcasestr(key.c_str(), "jump")) {
      state = CharacterAnimState::JUMP;
    } else {
      continue;
    }

    if (anim_frames.find(state) == anim_frames.end()) {
      anim_frames[state] = std::vector<CharacterAnimFrame>();
    }

    CharacterAnimFrame frame;
    frame.spritesheet_x = value[0].get<int>();
    frame.spritesheet_y = value[1].get<int>();
    frame.spritesheet_w = value[2].get<int>();
    frame.spritesheet_h = value[3].get<int>();
    frame.flip = false;

    anim_frames[state].push_back(frame);
  }

  // Copy size from first frame of stand
  if (anim_frames.find(CharacterAnimState::STAND) != anim_frames.end() &&
      anim_frames[CharacterAnimState::STAND].size() > 0) {
    auto &frame = anim_frames[CharacterAnimState::STAND][0];
    character_aabb = AABB::from_min_and_size(
        Vec2(0, 0), Vec2(frame.spritesheet_w, frame.spritesheet_h));
  } else {
    character_aabb = AABB::from_min_and_size(Vec2(0, 0), Vec2(70, 70));
  }
}

void CharacterEntity::update(Game &game, float delta_time) {
  this->controller->update(game, *this, delta_time);
  PhysEntity::update(game, delta_time);
  anim_frame_timer += delta_time;
  if (anim_frame_timer >= anim_frame_duration) {
    anim_frame_timer = 0.0f;
    anim_frame++;
  }
}

void CharacterEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_id);

  auto &frames = anim_frames[anim_state];

  auto &frame = frames[anim_frame % frames.size()];

  Rectangle src_rect = {static_cast<float>(frame.spritesheet_x),
                        static_cast<float>(frame.spritesheet_y),
                        static_cast<float>(frame.spritesheet_w),
                        static_cast<float>(frame.spritesheet_h)};
  Rectangle dest_rect = {this->position.x, this->position.y,
                         static_cast<float>(frame.spritesheet_w),
                         static_cast<float>(frame.spritesheet_h)};
  bool flip = frame.flip;
  if (this->direction == CharacterDirection::LEFT) {
    flip = !flip;
  }
  if (flip) {
    src_rect.width *= -1;
  }
  DrawTexturePro(*tex, src_rect, dest_rect, {0, 0}, 0.0f, WHITE);
#endif
}

void CharacterEntity::perform_movement(const Game &game, float delta_time,
                                       CharacterMovementCommand command) {
  bool on_ground_or_ladder = false;

  Vec2 feet_pos = this->position +
                  Vec2((this->get_aabb().min.x + this->get_aabb().max.x) / 2.0f,
                       this->get_aabb().max.y + 1.0f);
  for (auto &entity : game.entities) {
    if (entity == nullptr || entity->id == this->id ||
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
    this->direction = CharacterDirection::LEFT;
    if (on_ground_or_ladder) {
      this->anim_state = CharacterAnimState::WALK;
    }
    this->velocity.x -= this->horiz_accel * delta_time;
    if (this->velocity.x < -this->max_horiz_speed) {
      this->velocity.x = -this->max_horiz_speed;
    }
  } else if (command & CharacterMovementCommand::MOVE_RIGHT) {
    this->direction = CharacterDirection::RIGHT;
    if (on_ground_or_ladder) {
      this->anim_state = CharacterAnimState::WALK;
    }
    this->velocity.x += this->horiz_accel * delta_time;
    if (this->velocity.x > this->max_horiz_speed) {
      this->velocity.x = this->max_horiz_speed;
    }
  } else {
    if (on_ground_or_ladder) {
      if (anim_state == CharacterAnimState::WALK ||
          anim_state == CharacterAnimState::JUMP) {
        anim_state = CharacterAnimState::STAND;
      }
      if (std::fabs(this->velocity.x) > PHYS_EPSILON) {
        float sign = this->velocity.x > 0 ? 1.0f : -1.0f;
        this->velocity.x -= sign * this->horiz_accel * delta_time;
        if (sign * this->velocity.x < 0) {
          this->velocity.x = 0;
        }
      }
    }
  }

  // overwrite anim state if jumping
  if (command & CharacterMovementCommand::JUMP && on_ground_or_ladder) {
    anim_state = CharacterAnimState::JUMP;
  }
}

void KeyboardCharacterController::update(Game &game, CharacterEntity &character,
                                         float delta_time) {

  if (game.is_server()) {
    LOG_WARN() << "KeyboardCharacterController::update: called on server"
               << std::endl;
  }
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

  if (IsKeyReleased(KEY_E)) {
    if (!game.is_server()) {

      capnp::MallocMessageBuilder message;
      auto base_msg = message.initRoot<net::BaseNetMessage>();
      auto interact = base_msg.initInteract();
      interact.setInteractorNetId(character.net_id);
      game.send_reliable_to_peer(0, message);
    }
  }

  character.perform_movement(game, delta_time, command);
#endif
}

void DumbAICharacterController::update(Game &game, CharacterEntity &character,
                                       float delta_time) {
  CharacterMovementCommand command = CharacterMovementCommand::NONE;
  time_since_last_jump += delta_time;
  if (dir_change_time > 0.0f) {
    dir_change_time -= delta_time;
  } else if (dir_change_time > -1000.0f) {
    dir_change_time = -10000.0f;
    moving_right = !moving_right;
    dwell_time = 1.5f;
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
        if (!tilemap->check_allow_jump(pos_to_check) &&
            dir_change_time <= 0.0f) {

          dwell_time = 1.0f;
          moving_right = !moving_right;
          break;
        }

        if (tilemap->check_allow_jump(head_pos_to_check) &&
            dir_change_time <= 0.0f && time_since_last_jump > 7.0f) {
          time_since_last_jump = 0.0f;
          command |= CharacterMovementCommand::JUMP;
          // moving_right = !moving_right;
          dir_change_time = 2.0f;
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

net::EntityType CharacterEntity::get_net_type() {
  return net::EntityType::CHARACTER;
}
