#ifndef PHYSENTITY_H_
#define PHYSENTITY_H_

#include "AABB.h"
#include "Entity.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Vec2.h"
#include "TilemapEntity.h"

namespace giewont {

/**
 * @brief Entity which has an AABB and gravity affected physics.
 */
class PhysEntity : public Entity {
public:
  PhysEntity();

  const char *get_type_name() const override { return "PhysEntity"; }

  Vec2 velocity = {0, 0};

  /**
   * @brief If set to false the entity will not be affected by physics.
   */
  bool is_kinematic = false;

  // Fall damage calculation

  /** @brief Whether the entity is on solid ground (or on a ladder if a character) */
  bool is_propped_by_level = true;
  Vec2 last_on_ground_position = Vec2(0, 0);
  Vec2 highest_off_ground_position = Vec2(0, 0);

  /** @brief Check whether the entity  */
  virtual bool check_is_propped(TilemapEntity *tilemap, Vec2 feet_pos);

  /**
   * @brief Called when the entity lands after being airborne
   * 
   * @param fall_delta Delta between the highest point and the landing point.
   */
  virtual void on_has_landed(Game &game, Vec2 fall_delta);

  virtual AABB &get_aabb();
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;
  void draw_debug(const Game &game) override;

  void update_from_sync_message(
      Game &game,
      const net::SyncEntityNetMessage::Reader &sync_message) override;

  void
  build_sync_message(Game &game, net::SyncEntityNetMessage::Builder &sync_message) override;

private:
  AABB _default_aabb = AABB(Vec2(0, 0), Vec2(70, 70));
  Vec2 _resolution_vector_debug = {0, 0};
};

} // namespace giewont
#endif // PHYSENTITY_H_
