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
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;
  void draw_debug(const Game &game) override;

  private:
    AABB _default_aabb = AABB(Vec2(0, 0), Vec2(70, 70));
    Vec2 _resolution_vector_debug = {0, 0};

};

} // namespace giewont
#endif // PHYSENTITY_H_
