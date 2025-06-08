#include "ParticleSystemEntity.h"
#include "DataBinder.h"
#include "Game.h"
#include "LevelLoader.h"
#include "Log.h"
#include "TilemapEntity.h"
#include "Vec2.h"
#include <numbers>
#include <stdexcept>

#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif

using namespace giewont;

GW_DATABINDER_DEFINE(
    ParticleSystemEntity,
    GW_DATABINDER_FIELD_RANGE(float, emission_rate, 0.0, 1000.0),
    GW_DATABINDER_FIELD_RANGE(float, initial_velocity_angle, 0.0,
                              2.0 * std::numbers::pi),
    GW_DATABINDER_FIELD(Vec2, initial_velocity),
    GW_DATABINDER_FIELD_RANGE(float, max_lifetime, 0.0, 1000.0),
    GW_DATABINDER_FIELD_RANGE(float, gravity_factor, 0.0, 3.0),
    GW_DATABINDER_FIELD_RANGE(float, system_lifetime, 0.0, 1000.0),
    GW_DATABINDER_FIELD_RANGE(float, particle_size, 0.0, 10.0),
    GW_DATABINDER_FIELD_RANGE(float, particle_size_variation, 0.0, 10.0));

ParticleSystemEntity::ParticleSystemEntity(const nlohmann::json &data)
    : Entity() {

  auto properties = LevelLoader::tmjPropertiesToObj(data["properties"]);

  if (!properties.contains("particle_texture_path") ||
      !properties["particle_texture_path"].is_string()) {
    throw std::runtime_error(
        "ParticleSystemEntity: particle_texture_path is not a string");
  }

  this->particle_texture_path =
      properties["particle_texture_path"].get<std::string>();
  if (properties["max_particles"].is_number_integer()) {
    this->max_particles = properties["max_particles"].get<size_t>();
  }
  if (properties["emission_rate"].is_number_float()) {
    this->emission_rate = properties["emission_rate"].get<float>();
  }
  if (properties["initial_velocity_angle"].is_number()) {
    this->initial_velocity_angle =
        properties["initial_velocity_angle"].get<float>();
  }
  if (properties["initial_velocity"].is_array() &&
      properties["initial_velocity"].size() == 2) {
    this->initial_velocity = {properties["initial_velocity"][0].get<float>(),
                              properties["initial_velocity"][1].get<float>()};
  }
  if (properties["max_lifetime"].is_number()) {
    this->max_lifetime = properties["max_lifetime"].get<float>();
  }

  if (properties["system_lifetime"].is_number_float()) {
    this->system_lifetime = properties["system_lifetime"].get<float>();
  }

  if (properties["gravity_factor"].is_number_float()) {
    this->gravity_factor = properties["gravity_factor"].get<float>();
  }

  if (properties["particle_size"].is_number_float()) {
    this->particle_size = properties["particle_size"].get<float>();
  }

  if (properties["particle_size_variation"].is_number_float()) {
    this->particle_size_variation =
        properties["particle_size_variation"].get<float>();
  }

  if (properties["velocity_variation"].is_number_float()) {
    this->velocity_variation = properties["velocity_variation"].get<float>();
  }

  if (properties["tilemap_collision_action"].is_string()) {
    std::string action_str =
        properties["tilemap_collision_action"].get<std::string>();
    if (action_str == "NONE") {
      this->tilemap_collision_action = ParticleTilemapCollisionAction::NONE;
    } else if (action_str == "KILL") {
      this->tilemap_collision_action = ParticleTilemapCollisionAction::KILL;
    } else if (action_str == "BOUNCE") {
      this->tilemap_collision_action = ParticleTilemapCollisionAction::BOUNCE;
    } else {
      throw std::runtime_error("Unknown tilemap_collision_action: " +
                               action_str);
    }
  }

  if (properties["fade_size"].is_boolean()) {
    this->fade_size = properties["fade_size"].get<bool>();
  }

  if (properties["fade_alpha"].is_boolean()) {
    this->fade_alpha = properties["fade_alpha"].get<bool>();
  }

  this->position.x = data["x"].get<float>();
  this->position.y = data["y"].get<float>();
}

void ParticleSystemEntity::load_assets(const Game &game) {
  _texture_id = game.rm->load_texture(this->particle_texture_path);
}

void ParticleSystemEntity::update(Game &game, float delta_time) {
  if (this->particles.size() != this->max_particles) {
    this->particles.resize(this->max_particles);
  }
  bool should_emit =
      this->emission_rate >= 0.0000001f && this->system_lifetime >= 0.0f &&
      (!this->attached_to_spawner || this->spawner_entity.valid(game));
  if (should_emit) {
    size_t particles_to_emit =
        static_cast<size_t>(this->emission_rate * delta_time);

    if (particles_to_emit == 0) {
      _time_since_last_emission += delta_time;
      if (_time_since_last_emission >= 1.0f / this->emission_rate) {
        particles_to_emit = 1;
        _time_since_last_emission = 0.0f;
      }
    }

    Vec2 spawn_pos = this->position;

    if (this->attached_to_spawner) {
      spawn_pos = this->spawner_entity.get(game).position;
    }
    for (size_t i = 0; i < particles_to_emit; i++) {
      for (auto &particle : this->particles) {
        if (std::isnan(particle.lifetime)) {
          // spawn in this slot
          particle.lifetime = this->max_lifetime;
          particle.total_lifetime = this->max_lifetime;
          particle.position = spawn_pos;

          if (this->particle_size_variation > 0.0000001f) {
            particle.size = this->particle_size +
                            (rand() / static_cast<float>(RAND_MAX)) *
                                this->particle_size_variation -
                            this->particle_size_variation / 2.0f;
          } else {
            particle.size = this->particle_size;
          }

          particle.velocity = this->initial_velocity.rotated(
              (rand() / static_cast<float>(RAND_MAX)) *
                  this->initial_velocity_angle -
              this->initial_velocity_angle / 2.0f);

          if (this->velocity_variation > 0.0000001f) {
            particle.velocity *= 1.0f +
                                 (rand() / static_cast<float>(RAND_MAX)) *
                                     this->velocity_variation -
                                 this->velocity_variation / 2.0f;
          }
          break;
        }
      }
    }
  }

  // Find a tilemap to collide with (we only collide with one tilemap, where the
  // particle system is)
  TilemapEntity *tilemap_for_system = nullptr;
  if (this->tilemap_collision_action != ParticleTilemapCollisionAction::NONE) {
    for (auto &entity : game.valid_entities()) {
      if (TilemapEntity *tilemap =
              dynamic_cast<TilemapEntity *>(entity.get())) {
        if (tilemap->is_point_in_tilemap_bounds(position)) {
          tilemap_for_system = tilemap;
          break;
        }
      }
    }
  }

  bool has_alive_particles = false;
  for (auto &particle : this->particles) {
    if (std::isnan(particle.lifetime)) {
      continue;
    }
    has_alive_particles = true;
    particle.lifetime -= delta_time;
    if (particle.lifetime <= 0.0f) {
      particle.lifetime = NAN;
      continue;
    }
    particle.velocity += game.get_gvar<Vec2>(GVarType::GRAVITY) * delta_time * this->gravity_factor;
    particle.position += particle.velocity * delta_time;

    if (tilemap_for_system != nullptr &&
        (this->max_lifetime - particle.lifetime) >
            this->particle_immunity_time) {
      auto particle_radius = particle.size * 70.0f * 0.7; // TOOD: compute

      TilemapCollisionManifold manifolds[4];
      size_t manifolds_count = tilemap_for_system->check_collision_circle(
          particle.position, particle_radius, manifolds, 4);

      for (size_t i = 0; i < manifolds_count; i++) {
        if (this->tilemap_collision_action ==
            ParticleTilemapCollisionAction::KILL) {
          particle.lifetime = NAN;
          break;
        }
        {
          // todo handle bounce
        }
      }
    }
  }

  this->system_lifetime -= delta_time;
  if (!has_alive_particles) {
    if (this->system_lifetime <= 0.0f) {
      this->destroy();
    }
    if (this->attached_to_spawner && !this->spawner_entity.valid(game)) {
      this->destroy();
    }
  }
}

void ParticleSystemEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_id);
  for (auto &particle : this->particles) {
    if (std::isnan(particle.lifetime)) {
      continue;
    }

    auto size = particle.size;

    if (this->fade_size) {
      size *= particle.lifetime / particle.total_lifetime;
      if (size < 0.0001f) {
        size = 0.0001f;
      }
    }

    Color tint = WHITE;
    if (this->fade_alpha) {
      tint.a = 255 * (particle.lifetime / particle.total_lifetime);
    }

    DrawTextureEx(*tex, particle.position.to_raylib(), 0.0f, size, tint);
  }
#endif
}
