#pragma once
#include "Entity.h"
#include "Game.h"

namespace giewont {

class ProjectileEntity : public Entity {
public:
  ProjectileEntity();
  const char *get_type_name() const override { return "ProjectileEntity"; }

  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  // netcode
  net::EntityType get_net_type() override;

  void
  build_sync_message(Game &game,
                     net::SyncEntityNetMessage::Builder &sync_message) override;

  void update_from_sync_message(
      Game &game,
      const net::SyncEntityNetMessage::Reader &sync_message) override;

  // Synced state

  Vec2 velocity = Vec2(0.0f, 0.0f);
  float total_lifetime = 5.0f;
  float lifetime_left = 5.0f;

private:
  res_id _texture_res_id;
  res_id _hit_particle_prefab_res_id;
};

} // namespace giewont
