#ifndef TILEMAPENTITY_H_
#define TILEMAPENTITY_H_

#include "AABB.h"
#include "Entity.h"
#include "Game.h"
#include "IVec2.h"
#include "ResourceManager.h"
#include "Vec2.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

namespace giewont {

enum class TileType { AIR, SOLID, LADDER, WATER };

class TilesetTileInfo {
public:
  TileType tile;
};

/**
 * @brief Utility class for storing tileset data.
 * It is unique per each tilemap.
 */
class TilesetData {
public:
  res_id tileset_res_id;
  res_id texture_res_id;

  // Data from the json
  int tile_width; // in pixels on the texture
  int tile_height;
  int spacing;
  int columns;

  int first_gid; // Data from the tilemap json (can change between levels for
                 // the same tileset)

  TilesetData(int fitst_gid);
  TilesetData(res_id tileset_res_id, res_id texture_res_id, int first_gid);
  std::vector<TilesetTileInfo> tile_info;

  void load_tileset_data(const Game &game);
  void draw_tile(const Game &game, int tile_id, int pos_x, int pos_y, Vec2 size,
                 Vec2 offset);
};

class TilemapCollisionManifold {
public:
  Vec2 normal = Vec2(0, 0);
  float penetration = 0.0f;
  TileType tile_type = TileType::AIR;
};

/**
 * @brief Entity for a loaded tilemap.
 * Vec2 are in world space, while IVec2 are in tilemap space.
 */
class TilemapEntity : public Entity {
public:
  /**
   * @brief Construct a new Tilemap Entity object
   *
   * @param level_parent_folder The folder from which the level is loaded.
   * @param tile_layer_data
   * @param level_data
   * @param game
   */
  TilemapEntity(const std::filesystem::path &level_parent_folder,
                const nlohmann::json &tile_layer_data,
                const nlohmann::json &level_data, const Game &game);

  const char *get_type_name() const override { return "TilemapEntity"; }

  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  std::vector<TilemapCollisionManifold> check_collision_aabb(const AABB &aabb);
  size_t check_collision_circle(const Vec2 &center, float radius,
                                TilemapCollisionManifold manifolds[],
                                size_t max_manifolds);

  TileType check_collision_point(const Vec2 &point);
  /// @brief Check if world space point is in tilemap bounds
  bool is_point_in_tilemap_bounds(const Vec2 &point);

  IVec2 world_pos_to_tilemap_pos(const Vec2 &point) {
    return IVec2((int)((point.x - this->position.x) / this->tile_size.x),
                 (int)((point.y - this->position.y) / this->tile_size.y));
  }

  /// @brief Get the world space position of the bottom center of a tile (pos is
  /// tilemap space)
  Vec2 get_tile_bottom_center_world_pos(IVec2 pos);

  std::optional<IVec2> get_nearest_walkable_tile_pos(const Vec2 &point);

  /// @brief Get the tile type at the given tilemap position (returns AIR if
  /// out)
  inline TileType get_tile_type_at(IVec2 pos) {
    if (pos.x < 0 || pos.y < 0 || pos.x >= this->tilemap_width ||
        pos.y >= this->tilemap_height) {
      return TileType::AIR;
    }
    return get_tile_info_for_tile_id(
               this->tilemap_data[pos.y * this->tilemap_width + pos.x])
        .tile;
  }

  inline bool is_tile_in_bounds(IVec2 pos) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < this->tilemap_width &&
           pos.y < this->tilemap_height;
  }

  Vec2 tile_size = Vec2(0.0f, 0.0f); // in game units
  int tilemap_width;                 // in tiles
  int tilemap_height;                // in tiles

private:
  std::vector<int> tilemap_data;

  std::vector<TilesetData> tilesets = {
      TilesetData(0)}; // 0 is the air tileset, it is always present

  TilesetData &get_tileset_for_tile_id(int tile_id);
  TilesetTileInfo &get_tile_info_for_tile_id(int tile_id);
};

} // namespace giewont

#endif // TILEMAPENTITY_H_
