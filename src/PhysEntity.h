#ifndef PHYSENTITY_H_
#define PHYSENTITY_H_

#include "Entity.h"
#include "Game.h"


namespace giewont {

/**
 * @brief Entity which has an AABB and gravity affected physics.
 */
class PhysEntity : public Entity {
public:
  PhysEntity();
  void load_assets(const Game &game);
  void update(const Game &game, float delta_time);
  void draw(const Game &game);
};

} // namespace giewont
#endif // PHYSENTITY_H_
