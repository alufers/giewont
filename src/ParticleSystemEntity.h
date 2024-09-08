#ifndef PERTICLESYSTEMENTITY_H_
#define PERTICLESYSTEMENTITY_H_

#include "Entity.h"
#include "ResourceManager.h"
#include "Vec2.h"
#include <nlohmann/json.hpp>
#include <numbers>
#include <vector>

#include "DataBinder.h"

namespace giewont {

enum class ParticleTilemapCollisionAction { NONE, KILL, BOUNCE };

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

  const char *get_type_name() const override { return "ParticleSystemEntity"; }

  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

public:
  std::string particle_texture_path;

  size_t max_particles = 500;

  GW_DATABINDER_DECLARE(ParticleSystemEntity);

  GW_DATABINDER_AUTO_INSPECTOR(klass);

  float emission_rate = 100.0f;

  /**
   * @brief The maximum angle that the initial velocity can be rotated from the
   * initial_velocity.
   */
  float initial_velocity_angle = 0.1f * std::numbers::pi;
  Vec2 initial_velocity = {0.0f, -300.0f};

  float max_lifetime = 1.0f;

  float gravity_factor = 1.0f;

  ParticleTilemapCollisionAction tilemap_collision_action =
      ParticleTilemapCollisionAction::KILL;

private:
  float _time_since_last_emission = 0.0f;
  std::vector<Particle> particles;
  res_id _texture_id;
};

} // namespace giewont

#endif // PERTICLESYSTEMENTITY_H_
