#ifndef TEAMBASEENTITY_H_
#define TEAMBASEENTITY_H_

#include "DataBinder.h"
#include "Entity.h"
#include "GameplayManager.h"
#include "PhysEntity.h"
#include "math/Color.h"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

namespace giewont {

enum class TeamBaseSpriteType { STAND, ORB };

class TeamBaseSprite {
public:
  int spritesheet_x;
  int spritesheet_y;
  int spritesheet_w;
  int spritesheet_h;
};

class TeamBaseEntity : public Entity {
public:
  TeamBaseEntity();
  TeamBaseEntity(nlohmann::json data);
  const char *get_type_name() const override { return "TeamBaseEntity"; }
  net::EntityType get_net_type() override { return net::EntityType::TEAM_BASE; }
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  void update_from_sync_message(
      Game &game,
      const net::SyncEntityNetMessage::Reader &sync_message) override;

  void
  build_sync_message(Game &game,
                     net::SyncEntityNetMessage::Builder &sync_message) override;

  int32_t get_z_index() override { return -1; }

  GColor color = GColor(1.0f, 0.0f, 0.0f, 1.0f);

  float activation_level = 0.0f;
  GameplayTeam team = GameplayTeam::UNKNOWN_TEAM;
  EntityRef captured_flag;

  Vec2 get_flag_spawn_pos();

  Vec2 get_flag_attachment_pos() override;

  GW_DATABINDER_DECLARE(TeamBaseEntity);
  GW_DATABINDER_AUTO_INSPECTOR(klass);

private:
  std::unordered_map<TeamBaseSpriteType, TeamBaseSprite> sprites;
  std::string _texture_path;
  res_id _texture_id;
  res_id _spritesheet_data_id;

  AABB base_aabb = AABB();

  void load_spritesheet_data(const Game &game);
};
} // namespace giewont

#endif // TEAMBASEENTITY_H_
