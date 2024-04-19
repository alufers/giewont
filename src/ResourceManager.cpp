#include "ResourceManager.h"
#include <fstream>
using giewont::ResourceManager;
using giewont::res_id;

ResourceManager::ResourceManager() {}

res_id ResourceManager::load_json(std::string path) {
  if (json_paths.find(path) != json_paths.end()) {
    return json_paths[path];
  }
  json_paths[path] = jsons.size();
  jsons.push_back(std::make_shared<nlohmann::json>());
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + path);
  }
  file >> *jsons.back();
  return json_paths[path];
}

std::shared_ptr<nlohmann::json> ResourceManager::get_json(res_id id) {
  return jsons[id];
}


