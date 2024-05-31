#ifndef RESOURCEMANAGER_H_
#define RESOURCEMANAGER_H_

#include <exception>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "raylib.h"

#ifndef GIEWONT_ASSETS_PATH
#define GIEWONT_ASSETS_PATH "assets/"
#endif

namespace giewont {

/**
 * @brief Rsource ID.
 */
using res_id = size_t;
class ResourceManager {
public:
  ResourceManager();
  /**
   * @todo Make it configurable via ENV.
   */
  std::string assets_path = GIEWONT_ASSETS_PATH;
  /**
   * @brief Loads a JSON file into the resource manager.
   *
   * A file loaded twice from the same path will have the same ID.
   *
   * @throws std::runtime_error if the file could not be opened.
   * @param path Path to the JSON file (relative to assets/ directory)
   * @return res_id ID of the loaded resource.
   */
  res_id load_json(std::string path);
  std::shared_ptr<nlohmann::json> get_json(res_id id);

  /**
   * @brief Loads a texture into the resource manager.
   * A file loaded twice from the same path will have the same ID. 
   *
   * @param path The path to the texture file.
   * @return res_id The ID of the loaded resource.
   */
  res_id load_texture(std::string path);
  std::shared_ptr<Texture2D> get_texture(res_id id);

private:
  std::map<std::string, res_id> json_paths;
  std::map<std::string, res_id> texture_paths;
  std::vector<std::shared_ptr<nlohmann::json>> jsons;
  std::vector<std::shared_ptr<Texture2D>> textures;
};
} // namespace giewont

#endif // RESOURCEMANAGER_H_
