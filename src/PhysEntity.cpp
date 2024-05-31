#include "PhysEntity.h"
#include "Vec2.h"
#include "raylib.h"

#include "Log.h"
#include "TilemapEntity.h"
#include <cmath>
#include <memory>
using namespace giewont;

PhysEntity::PhysEntity() : Entity() {}

void PhysEntity::load_assets(const Game &game) {
  this->_sprite_id = game.rm->load_texture("entites/slime.png");
}

void PhysEntity::update(const Game &game, float delta_time) {
  this->velocity += game.gravity * delta_time;

  this->position += this->velocity * delta_time;

  for (auto &entity : game.entities) {
    if (entity->id == this->id || entity == nullptr ||
        entity->marked_for_deletion) {
      continue;
    }
    auto aabb_to_check = this->get_aabb().translated(this->position);
    if (TilemapEntity *tilemap = dynamic_cast<TilemapEntity *>(entity.get())) {
      Vec2 resolution(0.0f, 0.0f);
      if (tilemap->check_collision_aabb(aabb_to_check, resolution)) {
        auto resolutionUnit = resolution.normalized();

        this->velocity.x =
            this->velocity.x * (1.0 - std::abs(resolutionUnit.x));
        this->velocity.y =
            this->velocity.y * (1.0 - std::abs(resolutionUnit.y));

        this->position += resolution;
      }
    }
  }
}

void PhysEntity::draw(const Game &game) {
  DrawTexture(*game.rm->get_texture(this->_sprite_id), this->position.x,
              this->position.y, WHITE);
}

AABB &PhysEntity::get_aabb() { return _default_aabb; }
