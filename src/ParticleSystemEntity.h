#ifndef PLAYERSPAWNENTITY_H_
#define PLAYERSPAWNENTITY_H_

#include "Entity.h"
#include "ResourceManager.h"
#include "Vec2.h"
#include <nlohmann/json.hpp>
#include <numbers>
#include <vector>

namespace giewont {

class Particle {
public:
    Particle() : lifetime(NAN), position({0.0f, 0.0f}), velocity({0.0f, 0.0f}) {}
    float lifetime; // if NaN, the particle is dead
    Vec2 position;
    Vec2 velocity;
};


class ParticleSystemEntity : public Entity {
public:
  explicit ParticleSystemEntity(const nlohmann::json &data);
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;


public:
    std::string particle_texture_path;

    size_t max_particles = 500;
    float emission_rate = 0.0f;
   
   /**
    * @brief The maximum angle that the initial velocity can be rotated from the initial_velocity.
    */
    float initial_velocity_angle = 2.0f * std::numbers::pi;
    Vec2 initial_velocity = {0.0f, 0.0f};

    float max_lifetime = 1.0f;

private:

    std::vector<Particle> particles;
    res_id _texture_id;
    
};



} // namespace giewont

#endif // PLAYERSPAWNENTITY_H_
