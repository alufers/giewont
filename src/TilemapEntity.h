#ifndef TILEMAPENTITY_H_
#define TILEMAPENTITY_H_

#include "Entity.h"
#include "Game.h"
#include "ResourceManager.h"
#include <string>
#include <vector>

namespace giewont {

enum class TileType { SOLID, LADDER };

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
  int tile_width;
  int tile_height;
  int spacing;
  int columns;

  int first_gid; // Data from the tilemap json (can change between levels for the same tileset)

  TilesetData(res_id tileset_res_id, res_id texture_res_id, int first_gid);
  std::vector<TilesetTileInfo> tile_info;

  void load_tileset_data(const Game &game);
  void draw_tile(const Game &game, int tile_id, int pos_x, int pos_y);
};

/**
 * @brief Entity for a loaded tilemap.
 */
class TilemapEntity : public Entity {
public:
  TilemapEntity(std::string tilemap_json_path);
  void load_assets(const Game &game) override;
  void update(const Game &game) override;
  void draw(const Game &game) override;

private:
  int tilemap_width;  // in tiles
  int tilemap_height; // in tiles
  std::vector<int> tilemap_data;

  std::string tilemap_json_path;
  res_id tilemap_res_id;
  std::unique_ptr<TilesetData> tileset_data = nullptr;
};

} // namespace giewont

#endif // TILEMAPENTITY_H_
