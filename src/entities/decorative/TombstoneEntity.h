#pragma once
#include "Game.h"
#include "GameplayManager.h"
#include "ParticleSystemEntity.h"
#include "PhysEntity.h"

namespace giewont {

class TombstoneEntity : public PhysEntity {
public:
  TombstoneEntity();
  const char *get_type_name() const override { return "TombstoneEntity"; }

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

  AABB &get_aabb() override { return tombstone_aabb; }
  // Synced variables
  float total_lifetime = 25.0f;
  float lifetime = total_lifetime;
  float fade_out_time = 1.0f;
  bool fading_out = false;
  uint32_t dead_player_peer_id = 0;

  // Config
  float message_duration = 5.0f;
  float respawn_delay = 10.0f;

  // Server side variables
  bool local_did_try_respawn = false;
  GameplayTeam dead_player_team = GameplayTeam::UNKNOWN_TEAM;


   // Client side variables
  /// @brief Did we as a client emit particles?
  bool local_did_emit_particles = false;

private:
 
  res_id _texture_id;
  res_id _destroy_particle_res_id;

  AABB tombstone_aabb;
};

} // namespace giewont
