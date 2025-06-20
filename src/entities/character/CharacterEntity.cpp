#include "CharacterEntity.h"
#include "AABB.h"

#include "CameraEntity.h"
#include "FlagEntity.h"
#include "Game.h"
#include "GameplayManager.h"
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
#include <math/RandUtil.h>
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

  PhysEntity::load_assets(game);
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
  characterData.setTeam(static_cast<uint32_t>(team));
  characterData.setName(nickname);
}

void CharacterEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  PhysEntity::update_from_sync_message(game, sync_message);
  auto characterData = sync_message.getExtraData().getCharacterData();

  if (game.my_peer_id != this->net_owner_peer_id || this->is_being_created) {
    anim_state =
        static_cast<CharacterAnimState>(characterData.getAnimationState());
    direction = static_cast<CharacterDirection>(characterData.getDirection());
    health = characterData.getHealth();
    max_health = characterData.getMaxHealth();
    immunity_time = characterData.getImmunityTime();
    team = static_cast<GameplayTeam>(characterData.getTeam());
    nickname = characterData.getName();
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
  }
  if (this->health > max_health) {
    this->health = max_health;
  }
  if (!game.is_server() && this->net_owner_peer_id == game.my_peer_id &&
      msg.getDamage() > 0) {
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
    } else if (string_contains_case_insensitive(key, "climb")) {
      state = CharacterAnimState::CLIMB;
    } else if (string_contains_case_insensitive(key, "swim")) {
      state = CharacterAnimState::SWIM;
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
  if (anim_frame_timer >= CharacterEntity::anim_frame_durations[anim_state]) {
    anim_frame_timer = 0.0f;
    anim_frame++;
  }
  if (this->net_owner_peer_id == game.my_peer_id) {
    immunity_time -= delta_time;
    if (immunity_time < 0.0f) {
      immunity_time = 0.0f;
    }
  }

  if (this->nickname == "" && !game.is_server() &&
      this->net_owner_peer_id == game.my_peer_id) {
    // Set own nickname
    this->nickname = game.local_player_name;
  }

  if (game.is_server()) {
    // Server side death check
    if (this->health <= 0) {
      for (auto &entity : game.valid_entities()) {
        if (GameplayManager *gpm =
                dynamic_cast<GameplayManager *>(entity.get())) {
          gpm->notify_player_died(game, get_ref());
          break;
        }
      }
      this->destroy();
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
  auto &aabb = get_aabb();
  return position + Vec2((aabb.min.x + aabb.max.x) / 2.0f, aabb.max.y + 1.0f);
}

bool CharacterEntity::check_is_propped(TilemapEntity *tilemap, Vec2 feet_pos) {
  TileType feet_tile = tilemap->check_collision_point(feet_pos);
  is_on_ladder = feet_tile == TileType::LADDER; // TODO: not mutate it here
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

  // Draw the name over the character
  // if (this->net_owner_peer_id == game.my_peer_id) {

  Color team_color = (team == GameplayTeam::RED_TEAM) ? RED : BLUE;
  int font_size = 20;
  float spacing = 1.0f;
  auto textSize = MeasureTextEx(GetFontDefault(), nickname.c_str(), font_size,
                                spacing); // Measure the text width
  Vec2 name_pos =
      this->position +
      Vec2(dest_rect.width / 2.0 - textSize.x / 2.0f, -textSize.y - 4.0f);

  // DrawText(nickname.c_str(), name_pos.x, name_pos.y, 20, WHITE);
  DrawTextEx(GetFontDefault(), nickname.c_str(), name_pos.to_raylib(),
             font_size, spacing, team_color);
  // }

  if (this->immunity_time > 0.0f) {
    Rectangle scaled_dest = dest_rect;

    Color tint = BLUE;
    tint.a = (uint8_t)(255.0 * (immunity_time / 3.0));
    DrawTexturePro(*highlight_tex, src_rect, scaled_dest, {0, 0}, 0.0f, tint);
  }

  DrawTexturePro(*tex, src_rect, dest_rect, {0, 0}, 0.0f, WHITE);

  this->controller->draw(game);
#endif
}
void CharacterEntity::draw_debug(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  PhysEntity::draw_debug(game);
  this->controller->draw_debug(game);
#endif
}

void CharacterEntity::draw_inspector_ui(Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  PhysEntity::draw_inspector_ui(game);

  ImGui::Text("Nickname: %s", nickname.c_str());
  this->controller->draw_inspector_ui(game);
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

  if (command & CharacterMovementCommand::JUMP && this->is_in_water) {
    this->velocity.y = -this->jump_speed;
  }

  auto calculated_max_horiz_speed = this->max_horiz_speed;

  if (is_in_water) {
    calculated_max_horiz_speed *= 0.5f;
  }

  if (command & CharacterMovementCommand::MOVE_LEFT) {
    this->direction = CharacterDirection::LEFT;
    if (this->is_propped_by_level) {
      this->anim_state = CharacterAnimState::WALK;
    }
    this->velocity.x -= this->horiz_accel * delta_time;
    if (this->velocity.x < calculated_max_horiz_speed) {
      this->velocity.x = -calculated_max_horiz_speed;
    }
  } else if (command & CharacterMovementCommand::MOVE_RIGHT) {
    this->direction = CharacterDirection::RIGHT;
    if (this->is_propped_by_level) {
      this->anim_state = CharacterAnimState::WALK;
    }
    this->velocity.x += this->horiz_accel * delta_time;
    if (this->velocity.x > calculated_max_horiz_speed) {
      this->velocity.x = calculated_max_horiz_speed;
    }
  } else {
    if (this->is_propped_by_level) {
      if (anim_state == CharacterAnimState::WALK ||
          anim_state == CharacterAnimState::SWIM ||
          anim_state == CharacterAnimState::JUMP ||
          (anim_state == CharacterAnimState::CLIMB || !is_on_ladder)) {
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
    if (this->is_in_water) {
      this->anim_state = CharacterAnimState::SWIM;
    }

    if (!is_on_ladder && anim_state == CharacterAnimState::CLIMB) {
      this->anim_state = CharacterAnimState::JUMP;
    }
  }

  // overwrite anim state if jumping
  if (command & CharacterMovementCommand::JUMP && this->is_propped_by_level) {
    anim_state =
        is_on_ladder ? CharacterAnimState::CLIMB : CharacterAnimState::JUMP;
  }
}

net::EntityType CharacterEntity::get_net_type() {
  return net::EntityType::CHARACTER;
}

Vec2 CharacterEntity::world_projectile_launch_pos() {
  // Launch from the center of the character
  return position + Vec2(60.0f, 70.0f);
}
