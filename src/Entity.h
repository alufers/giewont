#ifndef ENTITY_H_
#define ENTITY_H_
#include <cstdint>
#include "Vec2.h"


namespace giewont {

class Game;
class Entity;
class EntityRef;

/**
 * @brief Base class for all entities in the game.
 *
 */
class Entity {
public:
  /** @brief Index of the entity in the main game entity vector. */
  uint32_t id;

  /** @brief Entity generation to compare when resolving EntityRefs */
  uint32_t generation;

  /** @brief Whether the entity should be deleted in the next frame. */
  bool marked_for_deletion = false;

  /** @brief Position in world-space of the entity. */
  Vec2 position = {0.0f, 0.0f};

  /**
   * @brief Load assets needed fro this entity.
   * 
   * @param game 
   */
  virtual void load_assets(const Game &game) {}

  /**
   * @brief Update entity state.
   *
   * @param game
   */
  virtual void update(const Game &game, float delta_time) = 0;

  /**
   * @brief Render the entity to the screen.
   *
   * @param game
   */
  virtual void draw(const Game &game) = 0;

  EntityRef get_ref() const;

  void destroy();
};

/**
 * @brief Reference to an entity that can survive multiple frames.
 */
class EntityRef {
public:
  uint32_t id = 0;
  uint32_t generation = 0;
  bool valid(Game const &game) const;
  Entity &get(Game &game) const;


  template <typename T>
  T &get_as(Game &game) const {
    return dynamic_cast<T &>(get(game));
  }

  template <typename T>
  bool valid_as(Game &game) const {
    return valid(game) && dynamic_cast<T *>(get(game)) != nullptr;
  }
};


} // namespace giewont

#endif // ENTITY_H_
