#include "ProjectileEntity.h"
#include "Entity.h"
#include "Log.h"
#include "TilemapEntity.h"
#include "entities/character/CharacterEntity.h"

using namespace giewont;
using namespace giewont::net;

ProjectileEntity::ProjectileEntity() : Entity() {}

void ProjectileEntity::load_assets(const Game &game) {
  Entity::load_assets(game);
  _texture_res_id = game.rm->load_texture("entities/projectile.png");
  _hit_particle_prefab_res_id = game.preload_prefab(
      "prefabs/character_fall_small.json"); // TODO: change me
}

void ProjectileEntity::update(Game &game, float delta_time) {
  this->position += velocity * delta_time;
  if (game.is_server()) {
    lifetime_left -= delta_time;
    if (lifetime_left <= 0.0f) {
      this->destroy();
    }
  }

  float continuous_dist = velocity.length() * delta_time + projectile_width;

  for (float i = 0.0f; i < continuous_dist; i += continuous_step) {
    Vec2 pos = position + velocity.normalized() * i;
    if (check_collision_at_pos(game, pos))
      break;
  }
}

bool ProjectileEntity::check_collision_at_pos(Game &game, Vec2 pos) {
  for (auto &entity : game.valid_entities()) {
    if (TilemapEntity *tm = dynamic_cast<TilemapEntity *>(entity.get())) {
      if (tm->check_collision_point(position) == TileType::SOLID) {
        this->destroy_or_hide(game);
        return true;
      }
    }
    if (game.is_server()) {
      if (PhysEntity *phys_ent = dynamic_cast<PhysEntity *>(entity.get())) {
        if (shooter_net_id != phys_ent->net_id) {
          auto aabb = phys_ent->get_aabb().translated(phys_ent->position);
          if (aabb.contains(pos)) {
            phys_ent->apply_impulse(game, velocity.normalized() * 500.0f);
            if (CharacterEntity *character =
                    dynamic_cast<CharacterEntity *>(phys_ent)) {
              character->hurt_entity(game, 15);
            }
            this->destroy_or_hide(game);
            return true;
          }
        }
      }
    }
  }
  return false;
}

void ProjectileEntity::destroy_or_hide(Game &game) {
  if (game.is_server()) {
    this->destroy();
    ::capnp::MallocMessageBuilder message;
    auto instantiate_message =
        message.initRoot<net::BaseNetMessage>().initInstantiatePrefab();
    instantiate_message.setPrefabPath("prefabs/projectile_hit.json");
    auto pos = instantiate_message.initPosition();
    position.serialize(pos);
    game.broadcast_reliable(message);
  } else {
    is_hidden_clientside = true;
  }
}

void ProjectileEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  if (is_hidden_clientside)
    return;
  auto tex = game.rm->get_texture(_texture_res_id);

  float rotation = 0.0f;
  if (velocity.x != 0.0f || velocity.y != 0.0f) {
    rotation = atan2(velocity.y, velocity.x) * RAD2DEG;
  }

  DrawTextureEx(*tex, position.to_raylib(), rotation, 1.0, WHITE);
#endif
}

EntityType ProjectileEntity::get_net_type() { return EntityType::PROJECTILE; }

void ProjectileEntity::build_sync_message(
    Game &game, net::SyncEntityNetMessage::Builder &sync_message) {
  Entity::build_sync_message(game, sync_message);
  auto projectileData = sync_message.initExtraData().initProjectileData();
  projectileData.setLifetime(lifetime_left);
  projectileData.setLifetime(total_lifetime);
  auto net_velocity = sync_message.initVelocity();
  velocity.serialize(net_velocity);
}

void ProjectileEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  Entity::update_from_sync_message(game, sync_message);
  auto projectileData = sync_message.getExtraData().getProjectileData();
  lifetime_left = projectileData.getLifetime();
  total_lifetime = projectileData.getLifetime();
  velocity = Vec2(sync_message.getVelocity());
}
