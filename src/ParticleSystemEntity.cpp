#include "ParticleSystemEntity.h"
#include <stdexcept>
#include <numbers>
#include "Game.h"
#include "LevelLoader.h"

#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif

using namespace giewont;

ParticleSystemEntity::ParticleSystemEntity(const nlohmann::json &data) : Entity() {

    auto properties = LevelLoader::tmjPropertiesToObj(data["properties"]);

  if(!properties.contains("particle_texture_path") || !properties["particle_texture_path"].is_string()) {
    throw std::runtime_error("ParticleSystemEntity: particle_texture_path is not a string");
  }

    this->particle_texture_path = properties["particle_texture_path"].get<std::string>();
    if(properties["max_particles"].is_number_integer()) {
        this->max_particles = properties["max_particles"].get<size_t>();
    }
    if(properties["emission_rate"].is_number_float()) {
        this->emission_rate = properties["emission_rate"].get<float>();
    }
    if(properties["initial_velocity_angle"].is_number_float()) {
        this->initial_velocity_angle = properties["initial_velocity_angle"].get<float>();
    }
    if(properties["initial_velocity"].is_array() && properties["initial_velocity"].size() == 2) {
        this->initial_velocity = {properties["initial_velocity"][0].get<float>(), properties["initial_velocity"][1].get<float>()};
    }
    if(properties["max_lifetime"].is_number_float()) {
        this->max_lifetime = properties["max_lifetime"].get<float>();
    }

}

void ParticleSystemEntity::load_assets(const Game &game) {
_texture_id = game.rm->load_texture(this->particle_texture_path);
}

void ParticleSystemEntity::update(Game &game, float delta_time) {
    if(this->particles.size() != this->max_particles) {
        this->particles.resize(this->max_particles);
    }
    for(auto &particle : this->particles) {
        if(std::isnan(particle.lifetime)) {
            continue;
        }
        particle.lifetime -= delta_time;
        if(particle.lifetime <= 0.0f) {
            particle.lifetime = NAN;
        }
        particle.position += particle.velocity * delta_time;
    }
}

void ParticleSystemEntity::draw(const Game &game) {
    #ifdef GIEWONT_HAS_GRAPHICS
    auto tex = game.rm->get_texture(_texture_id);
    for(auto &particle : this->particles) {
        if(std::isnan(particle.lifetime)) {
            continue;
        }
        DrawTexture(*tex, particle.position.x, particle.position.y, WHITE);
    }
    #endif
}
