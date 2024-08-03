#ifndef LEVELLOADER_H_
#define LEVELLOADER_H_

#include <string>
#include "ResourceManager.h"
#include <nlohmann/json.hpp>

namespace giewont {

// Forward declaration of Game class
class Game;

class LevelLoader {
public:
  LevelLoader(std::string tmj_path);

  void load_level(Game &game);

  static nlohmann::json tmjPropertiesToObj(const nlohmann::json &tmj_properties);
private:
  std::string tmj_path;
  res_id level_res_id;
};

} // namespace giewont

#endif // LEVELLOADER_H_
