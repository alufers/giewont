#ifndef GAME_H_
#define GAME_H_

#include "Entity.h"
#include <memory>
#include <vector>

namespace giewont {

class Game {

private:
  uint32_t generation_counter = 0;
  std::vector<std::unique_ptr<Entity>> entities;

public:
  static constexpr size_t MAX_ENTITIES = 1024;

  void update(float delta_time);
  void draw();

  void push_entity(std::unique_ptr<Entity> entity);

private:
  void destroy_marked_entities();
};

} // namespace giewont

#endif // GAME_H_
