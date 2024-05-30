#include "ResourceManager.h"
#include <fstream>
using giewont::ResourceManager;
using giewont::res_id;

ResourceManager::ResourceManager() {}

res_id ResourceManager::load_json(std::string path) {
  std::string full_path = assets_path + path;
  if (json_paths.find(full_path) != json_paths.end()) {
    return json_paths[full_path];
  }
  json_paths[full_path] = jsons.size();
  jsons.push_back(std::make_shared<nlohmann::json>());
  std::ifstream file(full_path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + full_path);
  }
  file >> *jsons.back();
  return json_paths[full_path];
}

std::shared_ptr<nlohmann::json> ResourceManager::get_json(res_id id) {
  return jsons[id];
}


res_id ResourceManager::load_texture(std::string path) {
  std::string full_path = assets_path + path;
  if (texture_paths.find(full_path) != texture_paths.end()) {
    return texture_paths[full_path];
  }
  texture_paths[full_path] = textures.size();
  textures.push_back(std::make_shared<Texture2D>(LoadTexture(full_path.c_str())));
  return texture_paths[full_path];
}

std::shared_ptr<Texture2D> ResourceManager::get_texture(res_id id) {
  return textures[id];
}
