#include "ParticleSystemEntity.h"
#include "DataBinder.h"
#include "Game.h"
#include "LevelLoader.h"
#include "Log.h"
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
    GW_DATABINDER_FIELD_RANGE(float, gravity_factor, 0.0, 3.0));

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
  if (properties["max_lifetime"].is_number_float()) {
    this->max_lifetime = properties["max_lifetime"].get<float>();
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
  if (this->emission_rate >= 0.0000001f) {
    size_t particles_to_emit =
        static_cast<size_t>(this->emission_rate * delta_time);

    if (particles_to_emit == 0) {
      _time_since_last_emission += delta_time;
      if (_time_since_last_emission >= 1.0f / this->emission_rate) {
        particles_to_emit = 1;
        _time_since_last_emission = 0.0f;
      }
    }

    for (size_t i = 0; i < particles_to_emit; i++) {
      for (auto &particle : this->particles) {
        if (std::isnan(particle.lifetime)) {
          particle.lifetime = this->max_lifetime;
          particle.position = Vec2(0.0, 0.0);

          particle.velocity = this->initial_velocity.rotated(
              (rand() / static_cast<float>(RAND_MAX)) *
                  this->initial_velocity_angle -
              this->initial_velocity_angle / 2.0f);
          break;
        }
      }
    }
  }

  for (auto &particle : this->particles) {

    if (std::isnan(particle.lifetime)) {
      continue;
    }
    particle.lifetime -= delta_time;
    if (particle.lifetime <= 0.0f) {
      particle.lifetime = NAN;
    }
    particle.velocity += game.gravity * delta_time * this->gravity_factor;
    particle.position += particle.velocity * delta_time;
  }
}

void ParticleSystemEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_id);
  for (auto &particle : this->particles) {
    if (std::isnan(particle.lifetime)) {
      continue;
    }

    DrawTexture(*tex, this->position.x + particle.position.x,
                this->position.y + particle.position.y, WHITE);
  }
#endif
}
