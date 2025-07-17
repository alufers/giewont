#ifndef GAME_H_
#define GAME_H_

#include "Entity.h"
#include "ResourceManager.h"
#include "Vec2.h"
#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif
#include "schema.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <memory>
#include <ranges>
#include <string>
#include <variant>
#include <vector>

#define PHYS_EPSILON 0.00001f

namespace giewont {

typedef giewont::net::GameplayVariable::GameplayVariable::Which GVarType;

class Game {

public:
  virtual ~Game() = default;

  Game();
  // General stuff
  static constexpr size_t MAX_ENTITIES = 2048;
  std::unique_ptr<ResourceManager> rm = std::make_unique<ResourceManager>();
  virtual EntityRef push_entity(std::unique_ptr<Entity> entity);

  // Camera
  EntityRef camera_ref;

  // Gameplay variables
  std::array<std::variant<float, Vec2>, GVarType::MAX> gameplay_variables;

  template <typename T> T get_gvar(GVarType type) const;

  void load_level(std::string tmj_path);
  virtual void update(float delta_time);

  // Multiplayer

  uint32_t my_peer_id = 0;
  virtual bool is_server() const { return false; }
  virtual void
  send_reliable_to_peer(uint32_t peer_id,
                        ::capnp::MallocMessageBuilder &message_builder) = 0;

  // virtual void handle_incoming_message(const net::BaseNetMessage::Reader
  // &message) = 0;

  virtual void
  broadcast_reliable(::capnp::MallocMessageBuilder &message_builder) = 0;

  /**
   * @brief Spawn a player character at this position for the given peer_id.
   * Server only.
   * @param peer_id
   * @param position
   * @return EntityRef
   */
  virtual EntityRef spawn_player_character(uint32_t peer_id, Vec2 position) = 0;

  virtual void shutdown() {};

  /**
   * @brief If client, send interaction to server. If server, perform the
   * interaction.
   *
   * @param message
   */
  virtual void
  perform_interaction(const net::InteractNetMessage::Reader &message) = 0;

// Font
#ifdef GIEWONT_HAS_GRAPHICS
  Font font;
#endif
  std::vector<std::unique_ptr<Entity>> entities;

  auto valid_entities() const {
    return entities | std::views::filter([](const auto &e) {
             return e != nullptr && e->id != 0 && !e->marked_for_deletion;
           });
  }

  EntityRef get_entity_by_net_id(uint32_t net_id);

  /** @brief Load a prefab json and load it's assets. */
  res_id preload_prefab(std::string prefab_path) const;

  EntityRef instantiate_prefab(res_id prefab_res);
  EntityRef instantiate_prefab(res_id prefab_res, Vec2 pos);

#ifdef GIEWONT_HAS_GRAPHICS

  virtual Camera2D get_currently_rendering_camera_data() const { return {0}; }
#endif

  std::string local_player_name = "Player";

protected:
  /** @brief Last update per second. */
  float last_ups = 0.0;

  virtual void
  apply_sync_entity(const net::SyncEntityNetMessage::Reader &message);
  void delete_marked_entities();

private:
  uint32_t generation_counter = 0;

  friend class EntityRef;
};

} // namespace giewont

#endif // GAME_H_
