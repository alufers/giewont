#include "TombstoneEntity.h"
#include "GameplayManager.h"
using namespace giewont;

TombstoneEntity::TombstoneEntity() : PhysEntity() {
  this->mass = 10000.0f; // Very large mass
}

void TombstoneEntity::load_assets(const Game &game) {
  PhysEntity::load_assets(game);
  tombstone_aabb = AABB(Vec2(0, 0), Vec2(71, 89));
  _texture_id = game.rm->load_texture("entities/tombstone.png");
  _destroy_particle_res_id =
      game.preload_prefab("prefabs/character_fall_small.json");
}

void TombstoneEntity::update(Game &game, float delta_time) {
  PhysEntity::update(game, delta_time);

  if (game.is_server()) {
    lifetime -= delta_time;
    if (lifetime <= 0.0f && !fading_out) {
      fading_out = true;
      lifetime = fade_out_time;
    }
    if (fading_out && lifetime <= 0.0f) {
      this->destroy();
    }

    if (!local_did_try_respawn && lifetime <= total_lifetime - respawn_delay) {
      local_did_try_respawn = true;
      for (auto &entity : game.valid_entities()) {
        if (GameplayManager *gpm =
                dynamic_cast<GameplayManager *>(entity.get())) {
          gpm->spawn_player_with_team(game, dead_player_peer_id,
                                      dead_player_team);
          break;
        }
      }
    }
  }
#if GIEWONT_HAS_GRAPHICS
  if (fading_out && !local_did_emit_particles) {
    local_did_emit_particles = true;
    game.instantiate_prefab(_destroy_particle_res_id,
                            position + tombstone_aabb.center());
  }

#endif
}

void TombstoneEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_id);
  Color tint = WHITE;
  if (fading_out) {
    tint.a = static_cast<unsigned char>(255 * (lifetime / fade_out_time));
  }
  DrawTextureV(*tex, position.to_raylib(), tint);
#endif
}

void TombstoneEntity::build_sync_message(
    Game &game, net::SyncEntityNetMessage::Builder &sync_message) {
  PhysEntity::build_sync_message(game, sync_message);
  auto tombstoneData = sync_message.initExtraData().initTombstoneData();
  tombstoneData.setTotalLifetime(total_lifetime);
  tombstoneData.setLifetime(lifetime);
  tombstoneData.setFadeOutTime(fade_out_time);
  tombstoneData.setFadingOut(fading_out);
  tombstoneData.setDeadPlayerPeerId(dead_player_peer_id);
}

void TombstoneEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  PhysEntity::update_from_sync_message(game, sync_message);
  auto tombstoneData = sync_message.getExtraData().getTombstoneData();
  total_lifetime = tombstoneData.getTotalLifetime();
  lifetime = tombstoneData.getLifetime();
  fade_out_time = tombstoneData.getFadeOutTime();
  fading_out = tombstoneData.getFadingOut();
  dead_player_peer_id = tombstoneData.getDeadPlayerPeerId();
}

net::EntityType TombstoneEntity::get_net_type() {
  return net::EntityType::TOMBSTONE;
}

void TombstoneEntity::draw_raylib_ui(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  if (dead_player_peer_id == game.my_peer_id) {
    if (lifetime > (total_lifetime - message_duration)) {
      std::string message = "You died!";
      auto message_text_x =
          GetScreenWidth() / 2 - MeasureText(message.c_str(), 40) / 2;

      Color message_color = RED;

      DrawText(message.c_str(), message_text_x, 240, 40, message_color);
    }
  }
#endif
}
