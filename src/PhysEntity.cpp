#include "PhysEntity.h"
#include "AABB.h"
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

void PhysEntity::load_assets(const Game &game) {

  _water_splash_prefab = game.preload_prefab("prefabs/water_splash.json");
}

void PhysEntity::update(Game &game, float delta_time) {
  this->_resolution_vector_debug = {0, 0};

  if (!is_kinematic) {
    this->velocity += game.gravity * delta_time;
    this->position += this->velocity * delta_time;
  }

  const AABB &own_aabb = this->get_aabb();
  Vec2 feet_pos =
      this->position +
      Vec2((own_aabb.min.x + own_aabb.max.x) / 2.0f, own_aabb.max.y + 1.0f);

  auto aabb_to_check = own_aabb.translated(this->position);
  bool has_landed = false;
  bool is_currently_in_water = false;
  bool has_fallen_into_water = false;
  Vec2 old_highest_off_ground_position = this->highest_off_ground_position;
  for (auto &entity : game.entities) {
    if (!entity || entity->id == this->id || entity->marked_for_deletion) {
      continue;
    }
    if (TilemapEntity *tilemap = dynamic_cast<TilemapEntity *>(entity.get())) {

      // Physics collision check
      auto collision_manifolds = tilemap->check_collision_aabb(aabb_to_check);
      for (auto &manifold : collision_manifolds) {
        if (manifold.tile_type == TileType::SOLID) {

          float velAlongNormal = this->velocity.dot(manifold.normal);
          if (velAlongNormal > 0) {
            continue;
          }
          float e = 0.5f;
          float j = -(1 + e) * velAlongNormal;
          float tilemapMass =
              std::numeric_limits<float>::infinity(); // We assume the tilemap
                                                      // is immovable
          j /= 1.0 / this->mass + 1.0 / tilemapMass;  // inverse mass
          Vec2 impulse = manifold.normal * j;
          if (!is_kinematic) {
            this->velocity += (1.0f / this->mass) * impulse;
            this->_resolution_vector_debug = impulse;
            this->position += manifold.normal * manifold.penetration * 1;
          }
        } else if (manifold.tile_type == TileType::WATER) {
          is_currently_in_water = true;
          this->velocity +=
              (Vec2(0, -35.0) * (manifold.penetration / 60.0f)) / mass;
          this->velocity *= (1.0f - 0.09f * (manifold.penetration / 60.0f));
        }
      }

      // Feet pos check
      if (this->check_is_propped(tilemap, feet_pos)) {
        has_landed = !this->is_propped_by_level;
        this->is_propped_by_level = true;
        this->last_on_ground_position = feet_pos;

        this->highest_off_ground_position = feet_pos; // reset that
      } else if (is_currently_in_water) {
        this->last_on_ground_position = feet_pos; // water resets fall damage
        this->is_propped_by_level = false;
        has_fallen_into_water = !this->is_in_water;
        this->is_in_water = true;
      } else {
        this->is_propped_by_level = false;
        if (this->highest_off_ground_position.y > feet_pos.y) {
          this->highest_off_ground_position = feet_pos;
        }
      }

      if (!is_currently_in_water) {
        this->is_in_water = false;
      }
    }
  }
  if (has_landed) {
    this->on_has_landed(game, feet_pos - old_highest_off_ground_position);
  }
  if (has_fallen_into_water) {
    this->on_fallen_into_water(game,
                               feet_pos - old_highest_off_ground_position);
  }
}

bool PhysEntity::check_is_propped(TilemapEntity *tilemap, Vec2 feet_pos) {
  return tilemap->check_collision_point(feet_pos) == TileType::SOLID;
}

void PhysEntity::on_has_landed(Game &game, Vec2 fall_delta) {}

void PhysEntity::on_fallen_into_water(Game &game, Vec2 fall_delta) {
#if GIEWONT_HAS_GRAPHICS
  Vec2 feet_pos = position + Vec2((get_aabb().min.x + get_aabb().max.x) / 2.0f,
                                  get_aabb().max.y + 1.0f);

  auto particle_sys_ref =
      game.instantiate_prefab(_water_splash_prefab, feet_pos);
#endif
}

void PhysEntity::draw(const Game &game) {}

void PhysEntity::draw_debug(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  // draw aabb
  auto aabb = this->get_aabb().translated(this->position);
  DrawRectangleLines(aabb.min.x, aabb.min.y, aabb.width(), aabb.height(),
                     this->is_propped_by_level ? GREEN : RED);
  // draw resolution vector
  Vec2 reso = this->_resolution_vector_debug * 10.0;

  char text_buffer[100];
  snprintf(text_buffer, 100, "V: %f %f", reso.x, reso.y);
  DrawText(text_buffer, this->position.x, this->position.y - 20, 10, RED);
  DrawLine(this->position.x, this->position.y, this->position.x + reso.x,
           this->position.y + reso.y, RED);

  if (!this->is_propped_by_level) {
    // Draw highest off ground position as a circle
    DrawCircle(this->highest_off_ground_position.x,
               this->highest_off_ground_position.y, 5, PURPLE);
  }
#endif
}

AABB &PhysEntity::get_aabb() { return _default_aabb; }

void PhysEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  Entity::update_from_sync_message(game, sync_message);
  if (game.my_peer_id != this->net_owner_peer_id || is_being_created) {

    this->velocity = sync_message.getVelocity();
  }
}

void PhysEntity::build_sync_message(
    Game &game, net::SyncEntityNetMessage::Builder &sync_message) {
  Entity::build_sync_message(game, sync_message);
  auto net_velocity = sync_message.initVelocity();
  velocity.serialize(net_velocity);
}

void PhysEntity::apply_impulse(Game &game, Vec2 impulse) {
  ::capnp::MallocMessageBuilder message;
  auto impulse_msg =
      message.initRoot<net::BaseNetMessage>().initApplyPhysicsImpulse();
  auto impulseVec = impulse_msg.initImpulse();
  impulse.serialize(impulseVec);
  impulse_msg.setNetId(net_id);

  if (this->net_owner_peer_id != game.my_peer_id) {
    game.broadcast_reliable(message);
  } else {
    this->handle_apply_impulse_message(game, impulse_msg);
  }
}

void PhysEntity::handle_apply_impulse_message(
    Game &game,
    const net::ApplyPhysicsImpulseNetMessage::Reader &apply_impulse_message) {
  Vec2 vec = Vec2(apply_impulse_message.getImpulse());
  LOG_DEBUG() << "Applying impulse to entity " << this->net_id << ", " << vec
              << std::endl;
  this->velocity += vec / this->mass;
}
