#pragma once
#include "AABB.h"
#include "Game.h"
#include "PhysEntity.h"

namespace giewont {

class GrenadeEntity : public PhysEntity {
public:
  GrenadeEntity();
  const char *get_type_name() const override { return "GrenadeEntity"; }

  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  AABB &get_aabb() override { return grenade_aab; }

  net::EntityType get_net_type() override;
  void
  build_sync_message(Game &game,
                     net::SyncEntityNetMessage::Builder &sync_message) override;

  void update_from_sync_message(
      Game &game,
      const net::SyncEntityNetMessage::Reader &sync_message) override;

  float fuse_total_time = 3.0f;
  float fuse_time_left = 3.0f;

  int hurt_damage = 40;
  float hurt_radius_close = 70.0;
  float hurt_radius_far = 210.0;

private:
  void explode(Game &game);
  res_id _texture_id;
  res_id _explosion_prefab_res_id;
  res_id _sparks_prefab_res_id;
  EntityRef sparks_entity;
  AABB grenade_aab;
};

} // namespace giewont
