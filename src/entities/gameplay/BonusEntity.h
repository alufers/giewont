#pragma once
#include "Game.h"
#include "GameplayManager.h"
#include "ParticleSystemEntity.h"
#include "PhysEntity.h"
#include "schema.capnp.h"
namespace giewont {

class BonusEntity : public PhysEntity {
public:
  BonusEntity();
  const char *get_type_name() const override { return "BonusEntity"; }

  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;
  void draw_raylib_ui(const Game &game) override;

  // netcode
  net::EntityType get_net_type() override;

  void
  build_sync_message(Game &game,
                     net::SyncEntityNetMessage::Builder &sync_message) override;

  void update_from_sync_message(
      Game &game,
      const net::SyncEntityNetMessage::Reader &sync_message) override;

  AABB &get_aabb() override { return bonus_aabb; }

  // Synced variables
  float total_lifetime = 60.0f;
  float lifetime = total_lifetime;
  float amount = 20.0f;

  net::BonusEntityType bonus_type = net::BonusEntityType::HEALTHKIT;

  bool handle_interaction(
      Game &game, const net::InteractNetMessage::Reader interaction) override;

  bool check_interaction_possible(Game &game, EntityRef interactor) override;

private:
  res_id _texture_id;
  res_id _destroy_particle_res_id;

  AABB bonus_aabb;
};

} // namespace giewont
