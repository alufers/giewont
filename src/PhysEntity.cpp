#include "PhysEntity.h"
#include "Entity.h"
#include "Log.h"
#include "TilemapEntity.h"
#include "Vec2.h"
#include <cmath>
#include <memory>
#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif

using namespace giewont;

PhysEntity::PhysEntity() : Entity() {}

void PhysEntity::load_assets(const Game &game) {}

void PhysEntity::update(Game &game, float delta_time) {
  this->_resolution_vector_debug = {0, 0};
  this->velocity += game.gravity * delta_time;
  this->position += this->velocity * delta_time;

  for (auto &entity : game.entities) {
    if (entity == nullptr || entity->id == this->id ||
        entity->marked_for_deletion) {
      continue;
    }
    auto aabb_to_check = this->get_aabb().translated(this->position);
    if (TilemapEntity *tilemap = dynamic_cast<TilemapEntity *>(entity.get())) {

      auto collision_manifolds = tilemap->check_collision_aabb(aabb_to_check);
      for (auto &manifold : collision_manifolds) {
        float velAlongNormal = this->velocity.dot(manifold.normal);
        if (velAlongNormal > 0) {
          continue;
        }
        float e = 0.1f;
        float j = -(1 + e) * velAlongNormal;
        j /= 1.0 / 1.0 + 1.0 / 1.0; // inverse mass
        Vec2 impulse = manifold.normal * j;

        this->velocity += impulse * 1.9; // 1.9 prevents oscillation
        this->_resolution_vector_debug = impulse;
        this->position += manifold.normal * manifold.penetration * 1;
      }
    }
  }
}

void PhysEntity::draw(const Game &game) {}

void PhysEntity::draw_debug(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  // draw aabb
  auto aabb = this->get_aabb().translated(this->position);
  DrawRectangleLines(aabb.min.x, aabb.min.y, aabb.width(), aabb.height(),
                     GREEN);
  // draw resolution vector
  Vec2 reso = this->_resolution_vector_debug * 10.0;

  char text_buffer[100];
  snprintf(text_buffer, 100, "V: %f %f", reso.x, reso.y);
  DrawText(text_buffer, this->position.x, this->position.y - 20, 10, RED);
  DrawLine(this->position.x, this->position.y, this->position.x + reso.x,
           this->position.y + reso.y, RED);
#endif
}

AABB &PhysEntity::get_aabb() { return _default_aabb; }

void PhysEntity::update_from_sync_message(
    Game const &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  Entity::update_from_sync_message(game, sync_message);
  if (game.my_peer_id != this->net_owner_peer_id || is_being_created) {

    this->velocity = sync_message.getVelocity();
  }
}

void PhysEntity::build_sync_message(
    net::SyncEntityNetMessage::Builder &sync_message) {
  Entity::build_sync_message(sync_message);
  auto net_velocity = sync_message.initVelocity();
  velocity.serialize(net_velocity);
}
