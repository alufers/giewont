#ifndef ENTITY_H_
#define ENTITY_H_
#include "AABB.h"
#include "Vec2.h"
#include "schema.capnp.h"
#include <cstdint>

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

  /** @brief Id used in the network protocol. */
  uint32_t net_id = 0;

  /**
   * @brief Peer id of the owner of this entity.
   * By default it's the server and the server has peer_id 0.
   */
  uint32_t net_owner_peer_id = 0;

  bool is_being_created = false;

  /** @brief Whether the entity should be deleted in the next frame. */
  bool marked_for_deletion = false;

  /** @brief Position in world-space of the entity. */
  Vec2 position = {0.0f, 0.0f};

  bool is_static = false;

  virtual const char *get_type_name() const;

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
  virtual void update(Game &game, float delta_time) = 0;

  /**
   * @brief Render the entity to the screen.
   *
   * @param game
   */
  virtual void draw(const Game &game) = 0;

  /**
   * @brief Function to draw debug information over all other entities.
   *
   * @param game
   */
  virtual void draw_debug(const Game &game) {}

  /**
   * @brief Get the z index of the entity. It will determine the order in which
   * entities are drawn.
   *
   * @return int32_t The z index.
   */
  virtual int32_t get_z_index() { return 0; }

  /**
   * @brief Draw controls when the entity is selected in the inspector.
   *
   * @param game
   */
  virtual void draw_inspector_ui(Game &game);

  /**
   * @brief Add data to the SyncEntityNetMessage.
   */
  virtual void
  build_sync_message(net::SyncEntityNetMessage::Builder &sync_message);

  /**
   * @brief Update this entity from a SyncEntityNetMessage.
   *
   * @param sync_message The message to update from.
   */
  virtual void update_from_sync_message(
      Game const &game, const net::SyncEntityNetMessage::Reader &sync_message);

  virtual bool
  handle_interaction(Game &game,
                     const net::InteractNetMessage::Reader interaction) {
    return false;
  }

  virtual net::EntityType get_net_type() { return net::EntityType::UNKNOWN; }

  EntityRef get_ref() const;

  void destroy();

  virtual ~Entity() = default;
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
  Entity *get_ptr(Game &game) const;

  template <typename T> T &get_as(Game &game) const {
    return dynamic_cast<T &>(get(game));
  }

  template <typename T> bool valid_as(Game &game) const {
    return valid(game) && dynamic_cast<T *>(get_ptr(game)) != nullptr;
  }

  bool operator==(const EntityRef &other) const {
    return id == other.id && generation == other.generation;
  }
};

} // namespace giewont

#endif // ENTITY_H_
