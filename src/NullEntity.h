#ifndef NULLENTITY_H_
#define NULLENTITY_H_

#include "Entity.h"
#include "Game.h"


namespace giewont {

/**
 * @brief Entity to occupy the zeroth index in the entity vector.
 */
class NullEntity : public Entity {
public:
  NullEntity(){
    this->generation = 99999;
  }
  void load_assets(const Game &game) override {};
  void update(Game &game, float delta_time) override {};
  void draw(const Game &game) override {};
};

} // namespace giewont
#endif // NULLENTITY_H_
