#ifndef GAME_H_
#define GAME_H_

#include "Entity.h"
#include "ResourceManager.h"
#include "Vec2.h"
#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif
#include "schema.capnp.h"
#include <memory>
#include <string>
#include <vector>

#define PHYS_EPSILON 0.00001f

namespace giewont {

class Game {

public:
  Game();
  // General stuff
  static constexpr size_t MAX_ENTITIES = 1024;
  std::unique_ptr<ResourceManager> rm = std::make_unique<ResourceManager>();

  // Camera
  EntityRef camera_ref;

  // Physics
  Vec2 gravity = {0.0f, 9.81f * 70}; // y is positive down, and 1m = 70 units

  // Debug
  bool debug_overlay = false;
  bool debug_ui = false;

  void load_level(std::string tmj_path);
  virtual void update(float delta_time);

  // Multiplayer

  uint32_t my_peer_id = 0;

  virtual bool is_server() const { return false; }

  virtual EntityRef push_entity(std::unique_ptr<Entity> entity);

  virtual void shutdown() {};

  std::vector<std::unique_ptr<Entity>> entities;

  EntityRef get_entity_by_net_id(uint32_t net_id);

#ifdef GIEWONT_HAS_GRAPHICS

  virtual Camera2D get_currently_rendering_camera_data() const { return {0}; }
#endif

protected:
  /** @brief Last update per second. */
  float last_ups = 0.0;

  virtual void
  apply_sync_entity(const net::SyncEntityNetMessage::Reader &message);

private:
  uint32_t generation_counter = 0;
  void destroy_marked_entities();

  friend class EntityRef;
};

} // namespace giewont

#endif // GAME_H_
