#ifndef RESOURCEMANAGER_H_
#define RESOURCEMANAGER_H_

#include <exception>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace giewont {

/**
 * @brief Rsource ID.
 */
using res_id = size_t;
class ResourceManager {
public:
  ResourceManager();

  /**
   * @brief Loads a JSON file into the resource manager.
   *
   * A file loaded twice from the same path will have the same ID.
   *
   * @throws std::runtime_error if the file could not be opened.
   * @param path Path to the JSON file.
   * @return res_id ID of the loaded resource.
   */
  res_id load_json(std::string path);
  std::shared_ptr<nlohmann::json> get_json(res_id id);

private:
  std::map<std::string, res_id> json_paths;
  std::vector<std::shared_ptr<nlohmann::json>> jsons;
};
} // namespace giewont

#endif // RESOURCEMANAGER_H_
