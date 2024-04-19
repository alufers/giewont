#ifndef ENTITY_H_
#define ENTITY_H_
#include <cstdint>

namespace giewont {

class Game;
class Entity;

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
  virtual void update(const Game &game) = 0;

  /**
   * @brief Render the entity to the screen.
   *
   * @param game
   */
  virtual void draw(const Game &game) = 0;

  void destroy();
};

/**
 * @brief Reference to an entity that can survive multiple frames.
 *
 */
class EntityRef {
public:
  uint32_t id;
  uint32_t generation;
};

} // namespace giewont

#endif // ENTITY_H_
