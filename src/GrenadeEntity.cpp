#include "GrenadeEntity.h"
#include "AABB.h"
#include "entities/character/CharacterEntity.h"
#include "ParticleSystemEntity.h"
#include "PhysEntity.h"
#include <cmath>
#ifdef GIEWONT_HAS_GRAPHICS
#include <raylib.h>
#endif

#include "Log.h"

using namespace giewont;

GrenadeEntity::GrenadeEntity() : PhysEntity() {
  this->collision_impulse_multiplier = 3.2f;
}
void GrenadeEntity::load_assets(const Game &game) {
  PhysEntity::load_assets(game);
  grenade_aab = AABB(Vec2(0, 0), Vec2(70, 70));
  _explosion_prefab_res_id =
      game.preload_prefab("prefabs/grenade_explosion.json");
  _sparks_prefab_res_id = game.preload_prefab("prefabs/grenade_sparks.json");
#ifdef GIEWONT_HAS_GRAPHICS
  _texture_id = game.rm->load_texture("entities/bomb.png");
#endif
}

void GrenadeEntity::update(Game &game, float delta_time) {
  PhysEntity::update(game, delta_time);

  fuse_time_left -= delta_time;
  if (fuse_time_left <= 0.0f && game.is_server()) {
    explode(game);
  }
  if (!game.is_server()) {
    if (!sparks_entity.valid(game)) {

      sparks_entity = game.instantiate_prefab(_sparks_prefab_res_id, position);

      auto &particle_system = sparks_entity.get_as<ParticleSystemEntity>(game);
      particle_system.attached_to_spawner = true;
      particle_system.spawner_entity = get_ref();
    }
  }
}

void GrenadeEntity::explode(Game &game) {

  ::capnp::MallocMessageBuilder message;
  auto instantiate_message =
      message.initRoot<net::BaseNetMessage>().initInstantiatePrefab();
  instantiate_message.setPrefabPath("prefabs/grenade_explosion.json");
  auto pos = instantiate_message.initPosition();
  position.serialize(pos);
  game.broadcast_reliable(message);

  for (auto &entity : game.valid_entities()) {
    if (CharacterEntity *character_ent =
            dynamic_cast<CharacterEntity *>(entity.get())) {
      auto dist = character_ent->position.distance(position);

      if (dist < hurt_radius_far) {
        auto dist_from_close = std::max(0.0f, dist - hurt_radius_close);
        auto damage_factor =
            1.0f - dist_from_close / (hurt_radius_far - hurt_radius_close);

        int damage = (int)((float)hurt_damage * damage_factor);
        LOG_INFO() << "Distance: " << dist
                   << " damage factor: " << damage_factor
                   << " damage: " << damage << std::endl;
        character_ent->hurt_entity(game, damage);
      }
    }

    if (PhysEntity *phys_ent = dynamic_cast<PhysEntity *>(entity.get())) {
      auto dist = phys_ent->position.distance(position);
      if (dist < hurt_radius_far) {
        auto dist_from_close = std::max(0.0f, dist - hurt_radius_close);
        auto damage_factor =
            1.0f - dist_from_close / (hurt_radius_far - hurt_radius_close);

        Vec2 direction = (phys_ent->position - (position + get_aabb().center()))
                             .normalized();

        Vec2 up_component = Vec2(0, 0);
        if (is_propped_by_level) {
          up_component = Vec2(0, -300.0f);
        }

        phys_ent->apply_impulse(game, direction * 1000.0f + up_component);
      }
    }
  }

  this->destroy();
}

void GrenadeEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_id);

  auto flash_fac =
      (1.0 + std::sin(fuse_time_left * (fuse_time_left < 1.3 ? 35.0 : 10.0))) /
      2.0;
  auto tint = ColorLerp(WHITE, RED, flash_fac);
  DrawTextureV(*tex, position.to_raylib(), tint);
#endif
}

net::EntityType GrenadeEntity::get_net_type() {
  return net::EntityType::GRENADE;
}

void GrenadeEntity::build_sync_message(
    Game &game, net::SyncEntityNetMessage::Builder &sync_message) {
  PhysEntity::build_sync_message(game, sync_message);
  auto grenadeData = sync_message.initExtraData().initGrenadeData();

  grenadeData.setFuseTimeLeft(fuse_time_left);
  grenadeData.setFuseTotalTime(fuse_total_time);
}

void GrenadeEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  PhysEntity::update_from_sync_message(game, sync_message);
  auto grenadeData = sync_message.getExtraData().getGrenadeData();
  fuse_time_left = grenadeData.getFuseTimeLeft();
  fuse_total_time = grenadeData.getFuseTotalTime();
}
