#include "CharacterEntity.h"
#include "AABB.h"

#include "CameraEntity.h"
#include "FlagEntity.h"
#include "Game.h"
#include "KeyboardCharacterController.h"
#include "Log.h"
#include "ParticleSystemEntity.h"
#include "PhysEntity.h"
#include "TilemapEntity.h"
#include "Util.h"
#include "schema.capnp.h"
#include <capnp/message.h>
#include <cmath>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>

#ifdef GIEWONT_HAS_GRAPHICS
#include <raylib.h>
#endif

using namespace giewont;

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
  _highlight_texture_id =
      game.rm->load_texture("entities/p1_higlight_spritesheet.png");
  _spritesheet_data_id = game.rm->load_json("entities/p1_spritesheet.json");
  _ui_bar_texture_id = game.rm->load_texture("ui_bar.png");
  _character_fall_particle_system_prefab_id =
      game.preload_prefab("prefabs/character_fall_small.json");

  load_spritesheet_data(game);
}

void CharacterEntity::build_sync_message(
    Game &game, net::SyncEntityNetMessage::Builder &sync_message) {
  PhysEntity::build_sync_message(game, sync_message);
  auto characterData = sync_message.initExtraData().initCharacterData();

  characterData.setAnimationState(static_cast<uint32_t>(anim_state));
  characterData.setDirection(static_cast<uint32_t>(direction));
  characterData.setHealth(health);
  characterData.setMaxHealth(max_health);
  characterData.setImmunityTime(immunity_time);
}

void CharacterEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  PhysEntity::update_from_sync_message(game, sync_message);
  auto characterData = sync_message.getExtraData().getCharacterData();

  if (game.my_peer_id != this->net_owner_peer_id) {
    anim_state =
        static_cast<CharacterAnimState>(characterData.getAnimationState());
    direction = static_cast<CharacterDirection>(characterData.getDirection());
    health = characterData.getHealth();
    max_health = characterData.getMaxHealth();
    immunity_time = characterData.getImmunityTime();
  }
}

void CharacterEntity::apply_hurt_message(
    Game &game, const net::HurtEntityNetMessage::Reader &msg) {
  if (immunity_time > 0.0f && msg.getDamage() > 0) {
    return;
  }

  this->health -= msg.getDamage();
  if (this->health < 0) {
    this->health = 0;
    // TODO: handle death
  }
  if (!game.is_server() && this->net_owner_peer_id == game.my_peer_id) {
    // Add some camera shake if we have been hurt
    std::unique_ptr<CameraShakeEffect> shake_effect =
        std::make_unique<CameraShakeEffect>();
    shake_effect->duration = 0.5f;
    shake_effect->duration_left = 0.5f;
    shake_effect->intensity =
        30.0f * ((float)msg.getDamage() / (float)max_health);
    shake_effect->falloff_time = 0.2f;
    shake_effect->speed = 150.0;
    game.camera_ref.get_as<CameraEntity>(game).effects.push_back(
        std::move(shake_effect));

    // If severe damage, add a red vignette effect
    if (msg.getDamage() > max_health * 0.15) {
      std::unique_ptr<VignetteEffect> vignette_effect =
          std::make_unique<VignetteEffect>();
      vignette_effect->duration = 0.5f;
      vignette_effect->duration_left = 0.5f;
      vignette_effect->intensity = 0.5f;
      vignette_effect->falloff_time = 0.2f;
      vignette_effect->color = GColor(1.0f, 0.0f, 0.0f);
      game.camera_ref.get_as<CameraEntity>(game).effects.push_back(
          std::move(vignette_effect));
    }
  }
}

void CharacterEntity::hurt_entity(Game &game, int damage) {
  ::capnp::MallocMessageBuilder message;
  auto hurt_msg = message.initRoot<net::BaseNetMessage>().initHurtEntity();
  hurt_msg.setDamage(damage);
  hurt_msg.setNetId(net_id);
  if (this->net_owner_peer_id == game.my_peer_id) {
    // Apply the hurt message
    apply_hurt_message(game, hurt_msg);
  } else {
    // Forward to owner
    game.send_reliable_to_peer(this->net_owner_peer_id, message);
  }
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
    if (string_contains_case_insensitive(key, "stand")) {
      state = CharacterAnimState::STAND;
    } else if (string_contains_case_insensitive(key, "walk")) {
      state = CharacterAnimState::WALK;
    } else if (string_contains_case_insensitive(key, "jump")) {
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
  if (this->net_owner_peer_id == game.my_peer_id) {
    immunity_time -= delta_time;
    if (immunity_time < 0.0f) {
      immunity_time = 0.0f;
    }
  }
}

void CharacterEntity::on_has_landed(Game &game, Vec2 fall_delta) {
  float fall_height = std::abs(fall_delta.y);
  if (fall_height > 100.0) {

#if GIEWONT_HAS_GRAPHICS
    Vec2 feet_pos =
        position + Vec2((get_aabb().min.x + get_aabb().max.x) / 2.0f,
                        get_aabb().max.y + 1.0f);

    auto particle_sys_ref = game.instantiate_prefab(
        _character_fall_particle_system_prefab_id, feet_pos);
#endif
  }

  // calculate fall damage if owner of the entity
  if (fall_height > min_fall_hurt_height &&
      game.my_peer_id == net_owner_peer_id) {
    float damage_factor =
        (fall_height - min_fall_hurt_height) / max_fall_hurt_height;
    int damage =
        static_cast<int>(damage_factor * fall_max_damage_factor * max_health);

    this->hurt_entity(game, damage);
  }
}

Vec2 CharacterEntity::world_feet_pos() {
  auto aabb = get_aabb();
  return position + Vec2((aabb.min.x + aabb.max.x) / 2.0f, aabb.max.y + 1.0f);
}

bool CharacterEntity::check_is_propped(TilemapEntity *tilemap, Vec2 feet_pos) {
  TileType feet_tile = tilemap->check_collision_point(feet_pos);
  return feet_tile == TileType::SOLID || feet_tile == TileType::LADDER;
}

void CharacterEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_id);
  auto highlight_tex = game.rm->get_texture(_highlight_texture_id);

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

  if (this->immunity_time > 0.0f) {
    Rectangle scaled_dest = dest_rect;
    // scaled_dest.width += 3.0;
    // scaled_dest.height += 3.0;
    // scaled_dest.x -= 3.0;
    // scaled_dest.y -= 3.0;
    Color tint = BLUE;
    tint.a = (uint8_t)(255.0 * (immunity_time / 3.0));
    DrawTexturePro(*highlight_tex, src_rect, scaled_dest, {0, 0}, 0.0f, tint);
  }

  DrawTexturePro(*tex, src_rect, dest_rect, {0, 0}, 0.0f, WHITE);
#endif
}

void CharacterEntity::draw_raylib_ui(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  if (this->net_owner_peer_id != game.my_peer_id) {
    return;
  }

  auto ui_bar_tex = game.rm->get_texture(_ui_bar_texture_id);

  Rectangle full_src_rect = {0, 0, (float)ui_bar_tex->width,
                             (float)ui_bar_tex->height};

  Rectangle health_src_rect = {
      0, 0, (float)ui_bar_tex->width * (float)health / (float)max_health,
      (float)ui_bar_tex->height};

  Rectangle dest_rect = {
      (float)(GetScreenWidth() - ui_bar_tex->width - 30),
      (float)(GetScreenHeight() - (float)ui_bar_tex->height - 30.0),
      (float)ui_bar_tex->width, (float)ui_bar_tex->height};

  Rectangle health_dest_rect = dest_rect;
  health_dest_rect.width = health_src_rect.width;

  Color bg_color = {255, 255, 255, 128};

  DrawTexturePro(*ui_bar_tex, full_src_rect, dest_rect, {0, 0}, 0.0f, bg_color);

  DrawTexturePro(*ui_bar_tex, health_src_rect, health_dest_rect, {0, 0}, 0.0f,
                 RED);

  char health_text[256];
  snprintf(health_text, 256, "Health: %d/%d", health, max_health);

  DrawText(health_text, dest_rect.x + 20,
           dest_rect.y + dest_rect.height / 2 - 10, 20, WHITE);

#endif
}

void CharacterEntity::perform_movement(const Game &game, float delta_time,
                                       CharacterMovementCommand command) {

  if (command & CharacterMovementCommand::JUMP && this->is_propped_by_level) {
    this->velocity.y = -this->jump_speed;
  }

  if (command & CharacterMovementCommand::MOVE_LEFT) {
    this->direction = CharacterDirection::LEFT;
    if (this->is_propped_by_level) {
      this->anim_state = CharacterAnimState::WALK;
    }
    this->velocity.x -= this->horiz_accel * delta_time;
    if (this->velocity.x < -this->max_horiz_speed) {
      this->velocity.x = -this->max_horiz_speed;
    }
  } else if (command & CharacterMovementCommand::MOVE_RIGHT) {
    this->direction = CharacterDirection::RIGHT;
    if (this->is_propped_by_level) {
      this->anim_state = CharacterAnimState::WALK;
    }
    this->velocity.x += this->horiz_accel * delta_time;
    if (this->velocity.x > this->max_horiz_speed) {
      this->velocity.x = this->max_horiz_speed;
    }
  } else {
    if (this->is_propped_by_level) {
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
  if (command & CharacterMovementCommand::JUMP && this->is_propped_by_level) {
    anim_state = CharacterAnimState::JUMP;
  }
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
