#ifndef PHYSENTITY_H_
#define PHYSENTITY_H_

#include "Entity.h"
#include "Game.h"
#include "AABB.h"
#include "ResourceManager.h"
#include "Vec2.h"


namespace giewont {

/**
 * @brief Entity which has an AABB and gravity affected physics.
 */
class PhysEntity : public Entity {
public:
  PhysEntity();

  Vec2 velocity = {0, 0};

  virtual AABB &get_aabb();
  void load_assets(const Game &game);
  void update(const Game &game, float delta_time);
  void draw(const Game &game);

  private:
    AABB _default_aabb = AABB(Vec2(0, 0), Vec2(70, 70));

    res_id _sprite_id;
};

} // namespace giewont
#endif // PHYSENTITY_H_
