#include "PhysEntity.h"
#include "Vec2.h"
#include "raylib.h"

#include "Log.h"
#include "TilemapEntity.h"
#include "raylib.h"
#include <cmath>
#include <memory>

using namespace giewont;

PhysEntity::PhysEntity() : Entity() {}

void PhysEntity::load_assets(const Game &game) {}

void PhysEntity::update(Game &game, float delta_time) {
  this->velocity += game.gravity * delta_time;

  this->position += this->velocity * delta_time;

  this->_resolution_vector_debug = {0, 0};

  for (auto &entity : game.entities) {
    if (entity->id == this->id || entity == nullptr ||
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
        j /= 1.0 / 1.0 + 1.0 / 1.0;
        Vec2 impulse = manifold.normal * j;
        this->velocity += impulse;
        this->_resolution_vector_debug = impulse;
        this->position += manifold.normal * manifold.penetration;
      }
    }
  }
}

void PhysEntity::draw(const Game &game) {}

void PhysEntity::draw_debug(const Game &game) {
  // draw aabb
  auto aabb = this->get_aabb().translated(this->position);
  DrawRectangleLines(aabb.min.x, aabb.min.y, aabb.width(), aabb.height(),
                     GREEN);
  // draw resolution vector
  Vec2 reso = this->_resolution_vector_debug * 10.0;
  LOG_DEBUG() << "Resolution vector: " << reso << std::endl;
  DrawLine(this->position.x, this->position.y, this->position.x + reso.x,
           this->position.y + reso.y, RED);
}

AABB &PhysEntity::get_aabb() { return _default_aabb; }
