#ifndef LEVELLOADER_H_
#define LEVELLOADER_H_

#include "Entity.h"
#include "ResourceManager.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace giewont {

// Forward declaration of Game class
class Game;

class LevelLoader {
public:
  LevelLoader(std::string tmj_path);

  void load_level(Game &game);

  /** @brief Initialize an Entity based on the json object for it (structured by
   * Tiled) */
  static std::unique_ptr<Entity>
  create_entity_from_json(const Game &game, const nlohmann::json &object);

  static nlohmann::json
  tmjPropertiesToObj(const nlohmann::json &tmj_properties);

private:
  std::string tmj_path;
  res_id level_res_id;
};

} // namespace giewont

#endif // LEVELLOADER_H_
