#include "TeamBaseEntity.h"
#include "Color.h"
#include "FlagEntity.h"
#include "PhysEntity.h"

#ifdef GIEWONT_HAS_GRAPHICS
#include "imgui.h"
#include <raylib.h>
#endif

using namespace giewont;

GW_DATABINDER_DEFINE(TeamBaseEntity, GW_DATABINDER_FIELD(GColor, color),
                     GW_DATABINDER_FIELD(float, activation_level));

TeamBaseEntity::TeamBaseEntity() : Entity() {}

TeamBaseEntity::TeamBaseEntity(nlohmann::json data) : Entity() {
  this->position.x = data["x"].get<float>();
  this->position.y = data["y"].get<float>();
}

void TeamBaseEntity::load_assets(const Game &game) {

  _texture_id = game.rm->load_texture("entities/team_base_spritesheet.png");
  _spritesheet_data_id =
      game.rm->load_json("entities/team_base_spritesheet.json");

  load_spritesheet_data(game);
}

void TeamBaseEntity::load_spritesheet_data(const Game &game) {
  auto data = game.rm->get_json(_spritesheet_data_id);

  // Check if it is a dictionary
  if (!data->is_object()) {
    throw std::runtime_error("Spritesheet data is not a dictionary");
  }

  for (auto &[key, value] : data->items()) {
    if (!value.is_array() || value.size() != 4) {
      throw std::runtime_error(
          "Spritesheet data is not an array of four elements (x, y, w, h)");
    }

    TeamBaseSpriteType state = TeamBaseSpriteType::STAND;
    if (strcasestr(key.c_str(), "orb")) {
      state = TeamBaseSpriteType::ORB;
    } else if (strcasestr(key.c_str(), "stand")) {
      state = TeamBaseSpriteType::STAND;
    } else {
      continue;
    }

    TeamBaseSprite sprite;
    sprite.spritesheet_x = value[0].get<int>();
    sprite.spritesheet_y = value[1].get<int>();
    sprite.spritesheet_w = value[2].get<int>();
    sprite.spritesheet_h = value[3].get<int>();

    sprites[state] = sprite;
  }

  if (sprites.find(TeamBaseSpriteType::STAND) != sprites.end()) {
    auto &spr = sprites[TeamBaseSpriteType::STAND];
    base_aabb = AABB::from_min_and_size(
        Vec2(0, 0), Vec2(spr.spritesheet_w, spr.spritesheet_h));
  } else {
    base_aabb = AABB::from_min_and_size(Vec2(0, 0), Vec2(140, 140));
  }
}

void TeamBaseEntity::update(Game &game, float delta_time) {

  float smallest_dist_to_flag = 999999.0f;
  for (auto &entity : game.valid_entities()) {
    if (FlagEntity *flag = dynamic_cast<FlagEntity *>(entity.get())) {
      float dist = flag->position.distance(position);
      if (dist < smallest_dist_to_flag) {
        smallest_dist_to_flag = dist;
      }
    }
  }

  float activation_dist = 70.0f * 2.0f;

  if (smallest_dist_to_flag < activation_dist) {
    float activation_from_dist = smallest_dist_to_flag / activation_dist;
    if (activation_level < activation_from_dist) {
      activation_level += delta_time * 0.2f;
    }
  } else {
    activation_level -= delta_time * 0.2f;
    if (activation_level < 0.0f) {
      activation_level = 0.0f;
    }
  }
}

void TeamBaseEntity::draw(const Game &game) {

#ifdef GIEWONT_HAS_GRAPHICS
  auto stand_spr = sprites[TeamBaseSpriteType::STAND];
  auto orb_spr = sprites[TeamBaseSpriteType::ORB];

  Rectangle src_rect_stand = {static_cast<float>(stand_spr.spritesheet_x),
                              static_cast<float>(stand_spr.spritesheet_y),
                              static_cast<float>(stand_spr.spritesheet_w),
                              static_cast<float>(stand_spr.spritesheet_h)};
  Rectangle src_rect_orb = {static_cast<float>(orb_spr.spritesheet_x),
                            static_cast<float>(orb_spr.spritesheet_y),
                            static_cast<float>(orb_spr.spritesheet_w),
                            static_cast<float>(orb_spr.spritesheet_h)};
  Rectangle dest_rect = {position.x, position.y - base_aabb.height(),
                         base_aabb.width(), base_aabb.height()};
  auto tex = game.rm->get_texture(_texture_id);

  GColor activation_color = color;

  activation_color.a = 0.4;

  DrawCircleV(position.to_raylib(), activation_level * 210.0f,
              activation_color.to_raylib_color());

  DrawTexturePro(*tex, src_rect_orb, dest_rect, {0, 0}, 0.0f,
                 color.to_raylib_color());
  DrawTexturePro(*tex, src_rect_stand, dest_rect, {0, 0}, 0.0f, WHITE);
#endif
}
