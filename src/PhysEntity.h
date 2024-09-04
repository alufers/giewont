#ifndef PHYSENTITY_H_
#define PHYSENTITY_H_

#include "AABB.h"
#include "Entity.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Vec2.h"

namespace giewont {

/**
 * @brief Entity which has an AABB and gravity affected physics.
 */
class PhysEntity : public Entity {
public:
  PhysEntity();

  const char *get_type_name() const override { return "PhysEntity"; }

  Vec2 velocity = {0, 0};

  virtual AABB &get_aabb();
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;
  void draw_debug(const Game &game) override;

  void update_from_sync_message(
      Game const &game,
      const net::SyncEntityNetMessage::Reader &sync_message) override;

private:
  AABB _default_aabb = AABB(Vec2(0, 0), Vec2(70, 70));
  Vec2 _resolution_vector_debug = {0, 0};
};

} // namespace giewont
#endif // PHYSENTITY_H_
