#ifndef TILEMAPENTITY_H_
#define TILEMAPENTITY_H_

#include "AABB.h"
#include "Entity.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Vec2.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <filesystem>

namespace giewont {

enum class TileType { AIR, SOLID, LADDER };

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
  Vec2 normal;
  float penetration;
};

/**
 * @brief Entity for a loaded tilemap.
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

  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  std::vector<TilemapCollisionManifold> check_collision_aabb(const AABB &aabb);

  bool check_allow_jump(const Vec2 &feet_pos);

private:
  int tilemap_width;  // in tiles
  int tilemap_height; // in tiles

  Vec2 tile_size = Vec2(0.0f, 0.0f); // in game units

  std::vector<int> tilemap_data;

  std::vector<TilesetData> tilesets = {
      TilesetData(0)}; // 0 is the air tileset, it is always present

  TilesetData &get_tileset_for_tile_id(int tile_id);
  TilesetTileInfo &get_tile_info_for_tile_id(int tile_id);
};

} // namespace giewont

#endif // TILEMAPENTITY_H_
