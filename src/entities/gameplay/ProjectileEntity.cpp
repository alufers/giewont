#include "ProjectileEntity.h"
#include "Entity.h"

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
}

void ProjectileEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_res_id);

  DrawTextureV(*tex, position.to_raylib(), WHITE);
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
  sync_message.setVelocity(net_velocity);
}

void ProjectileEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  Entity::update_from_sync_message(game, sync_message);
  auto projectileData = sync_message.getExtraData().getProjectileData();
  lifetime_left = projectileData.getLifetime();
  total_lifetime = projectileData.getLifetime();
  velocity = Vec2(sync_message.getVelocity());
}
