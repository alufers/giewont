#ifndef TILEMAPENTITY_H_
#define TILEMAPENTITY_H_

#include "Entity.h"
#include "Game.h"
#include "ResourceManager.h"
#include <string>

namespace giewont {
/**
 * @brief Entity for the loaded tilemap.
 *
 */
class TilemapEntity : Entity {
public:
  TilemapEntity(std::string tilemap_json_path);

  void load_assets(const Game &game);

  void update(const Game &game) = 0;
  void draw(const Game &game) = 0;

private:
  std::string tilemap_json_path;
  res_id tilemap_res_id;
  res_id tileset_res_id;
};

} // namespace giewont

#endif // TILEMAPENTITY_H_
