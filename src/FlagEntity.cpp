#include "FlagEntity.h"
#include "Log.h"
#include "PhysEntity.h"
#ifdef GIEWONT_HAS_GRAPHICS
#include "imgui.h"
#include <raylib.h>
#endif
#include "Util.h"

using namespace giewont;

GW_DATABINDER_DEFINE(FlagEntity, GW_DATABINDER_FIELD(GColor, color),
                     GW_DATABINDER_FIELD(GameplayTeam, team));

FlagEntity::FlagEntity() : PhysEntity() {}

FlagEntity::FlagEntity(nlohmann::json data) : PhysEntity() {
  this->position.x = data["x"].get<float>();
  this->position.y = data["y"].get<float>();
}

void FlagEntity::load_assets(const Game &game) {
  PhysEntity::load_assets(game);
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
    if (string_contains_case_insensitive(key, "pole")) {
      state = FlagEntitySpriteType::POLE;
    } else if (string_contains_case_insensitive(key, "waving")) {
      state = FlagEntitySpriteType::WAVING;
    } else if (string_contains_case_insensitive(key, "deform_up")) {
      state = FlagEntitySpriteType::DEFORM_UP;
    } else if (string_contains_case_insensitive(key, "deform_down")) {
      state = FlagEntitySpriteType::DEFORM_DOWN;
    } else {
      continue;
    }

    if (sprites.find(state) == sprites.end()) {
      sprites[state] = std::vector<FlagEntitySprite>();
    }

    FlagEntitySprite frame;
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
  if (flag_holder.valid(game)) {
    position = flag_holder.get(game).get_flag_attachment_pos();
    if (flag_holder.valid_as<PhysEntity>(game)) {
      velocity = flag_holder.get_as<PhysEntity>(game).velocity;
    }
  }

  needs_flip = velocity.x < 0;

  if (velocity.y < -30.0f) {
    needs_deform_up = true;
    needs_deform_down = false;
  } else if (velocity.y > 30.0f) {
    needs_deform_up = false;
    needs_deform_down = true;
  } else {
    needs_deform_up = false;
    needs_deform_down = false;
  }

  anim_frame_timer += delta_time;
  if (anim_frame_timer > 2.0f) {
    anim_frame_timer = 0.0f;
    anim_frame =
        (anim_frame + 1) % sprites[FlagEntitySpriteType::WAVING].size();
  }
}

void FlagEntity::draw(const Game &game) {

#ifdef GIEWONT_HAS_GRAPHICS
  auto pole_spr = sprites[FlagEntitySpriteType::POLE][0];

  auto wave_spr =
      sprites[FlagEntitySpriteType::WAVING]
             [anim_frame % sprites[FlagEntitySpriteType::WAVING].size()];

  if (needs_deform_up) {
    wave_spr = sprites[FlagEntitySpriteType::DEFORM_UP][0];
  } else if (needs_deform_down) {
    wave_spr = sprites[FlagEntitySpriteType::DEFORM_DOWN][0];
  }

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

  if (needs_flip) {
    src_rect_pole.width *= -1;
    src_rect_wave.width *= -1;
    dest_rect.x -= get_aabb().width() - 6.0;
  }
  auto tex = game.rm->get_texture(_texture_id);

  DrawTexturePro(*tex, src_rect_pole, dest_rect, {0, 0}, 0.0f, WHITE);
  DrawTexturePro(*tex, src_rect_wave, dest_rect, {0, 0}, 0.0f,
                 color.to_raylib_color());
#endif
}

bool FlagEntity::handle_interaction(
    Game &game, const net::InteractNetMessage::Reader interaction) {
      
  EntityRef interactor =
      game.get_entity_by_net_id(interaction.getInteractorNetId());
  if (flag_holder.valid(game)) {
    if (flag_holder == interactor) {
      flag_holder = EntityRef();
      this->velocity *= 2; // Throw the flag
      return true;
    }
    return false;
  }

  auto dist = (interactor.get(game).position - position).length();

  if (dist > game.get_gvar<float>(GVarType::ENTITY_INTERACTION_RANGE)) {
    return false;
  }

  

  flag_holder = interactor;
  return true;
}

void FlagEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  Entity::update_from_sync_message(game, sync_message);

  if (sync_message.getExtraData().which() !=
      net::SyncEntityNetMessage::ExtraData::Which::FLAG_DATA) {
    LOG_WARN() << "FlagEntity: ExtraData is not FLAG_DATA, but"
               << sync_message.getExtraData().which() << std::endl;
    throw std::runtime_error("FlagEntity: ExtraData is not FLAG_DATA");
  }

  auto flag_data = sync_message.getExtraData().getFlagData();
  team = static_cast<GameplayTeam>(flag_data.getTeam());
  color = GColor::from_uint32(flag_data.getColor());

  flag_holder = game.get_entity_by_net_id(flag_data.getHolderNetId());
}

void FlagEntity::build_sync_message(
    Game &game, net::SyncEntityNetMessage::Builder &sync_message) {
  PhysEntity::build_sync_message(game, sync_message);

  auto flag_data = sync_message.getExtraData().initFlagData();
  flag_data.setTeam(static_cast<uint32_t>(team));
  flag_data.setColor(color.to_uint32());
  flag_data.setHolderNetId(
      flag_holder.valid(game) ? flag_holder.get(game).net_id : 0);
}
