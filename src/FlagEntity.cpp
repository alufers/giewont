#include "FlagEntity.h"

#ifdef GIEWONT_HAS_GRAPHICS
#include "imgui.h"
#include <raylib.h>
#endif

using namespace giewont;


GW_DATABINDER_DEFINE(FlagEntity, GW_DATABINDER_FIELD(GColor, color));

FlagEntity::FlagEntity() : PhysEntity() {}

FlagEntity::FlagEntity(nlohmann::json data) : PhysEntity() {
  this->position.x = data["x"].get<float>();
  this->position.y = data["y"].get<float>();
}

void FlagEntity::load_assets(const Game &game) {

  _texture_id = game.rm->load_texture("entities/flag_spritesheet.png");
  _spritesheet_data_id = game.rm->load_json("entities/flag_spritesheet.json");

  load_spritesheet_data(game);
}

void FlagEntity::load_spritesheet_data(const Game &game) {
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

    FlagEntitySpriteType state = FlagEntitySpriteType::WAVING;
    if (strcasestr(key.c_str(), "pole")) {
      state = FlagEntitySpriteType::POLE;
    } else if (strcasestr(key.c_str(), "waving")) {
      state = FlagEntitySpriteType::WAVING;
    } else if (strcasestr(key.c_str(), "deform_up")) {
      state = FlagEntitySpriteType::DEFORM_UP;
    } else {
      continue;
    }

    if (sprites.find(state) == sprites.end()) {
      sprites[state] = std::vector<FLagEntitySprite>();
    }

    FLagEntitySprite frame;
    frame.spritesheet_x = value[0].get<int>();
    frame.spritesheet_y = value[1].get<int>();
    frame.spritesheet_w = value[2].get<int>();
    frame.spritesheet_h = value[3].get<int>();

    sprites[state].push_back(frame);
  }

  // Copy size from first frame of stand
  if (sprites.find(FlagEntitySpriteType::POLE) != sprites.end() &&
      sprites[FlagEntitySpriteType::POLE].size() > 0) {
    auto &spr = sprites[FlagEntitySpriteType::POLE][0];
    flag_aabb = AABB::from_min_and_size(
        Vec2(0, 0), Vec2(spr.spritesheet_w, spr.spritesheet_h));
  } else {
    flag_aabb = AABB::from_min_and_size(Vec2(0, 0), Vec2(70, 70));
  }
}

AABB &FlagEntity::get_aabb() { return flag_aabb; }

void FlagEntity::update(Game &game, float delta_time) {
  PhysEntity::update(game, delta_time);
}

void FlagEntity::draw(const Game &game) {

#ifdef GIEWONT_HAS_GRAPHICS
  auto pole_spr = sprites[FlagEntitySpriteType::POLE][0];

  auto wave_spr = sprites[FlagEntitySpriteType::WAVING][0];

  Rectangle src_rect_pole = {static_cast<float>(pole_spr.spritesheet_x),
                             static_cast<float>(pole_spr.spritesheet_y),
                             static_cast<float>(pole_spr.spritesheet_w),
                             static_cast<float>(pole_spr.spritesheet_h)};
  Rectangle src_rect_wave = {static_cast<float>(wave_spr.spritesheet_x),
                             static_cast<float>(wave_spr.spritesheet_y),
                             static_cast<float>(wave_spr.spritesheet_w),
                             static_cast<float>(wave_spr.spritesheet_h)};
  Rectangle dest_rect = {position.x, position.y, get_aabb().width(),
                         get_aabb().height()};
  auto tex = game.rm->get_texture(_texture_id);

  DrawTexturePro(*tex, src_rect_pole, dest_rect, {0, 0}, 0.0f, WHITE);
  DrawTexturePro(*tex, src_rect_wave, dest_rect, {0, 0}, 0.0f,
                 color.to_raylib_color());
#endif
}
