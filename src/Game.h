#ifndef GAME_H_
#define GAME_H_

#include "Entity.h"
#include "ResourceManager.h"
#include "Vec2.h"
#include <memory>
#include <vector>

#define PHYS_EPSILON 0.00001f

namespace giewont {

class Game {

public:
  Game();
  // General stuff
  static constexpr size_t MAX_ENTITIES = 1024;
  std::unique_ptr<ResourceManager> rm = std::make_unique<ResourceManager>();

  // Camera
  EntityRef camera_ref;

  // Physics
  Vec2 gravity = {0.0f, 9.81f * 70}; // y is positive down, and 1m = 70 units

  void load_assets();
  void update(float delta_time);
  void draw();

  EntityRef push_entity(std::unique_ptr<Entity> entity);

  std::vector<std::unique_ptr<Entity>> entities;

private:
  uint32_t generation_counter = 0;
  void destroy_marked_entities();

  /** @brief Last update per second. */
  float last_ups = 0.0;

  friend class EntityRef;
};

} // namespace giewont

#endif // GAME_H_
