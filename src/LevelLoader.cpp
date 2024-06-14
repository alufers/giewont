#include "LevelLoader.h"
#include "Game.h"
#include "TilemapEntity.h"
#include <filesystem>
#include <nlohmann/json.hpp>

using namespace giewont;

LevelLoader::LevelLoader(std::string tmj_path) : tmj_path(tmj_path) {}

void LevelLoader::load_level(Game &game) {
  this->level_res_id = game.rm->load_json(this->tmj_path);
  auto levelData = game.rm->get_json(this->level_res_id);

  for (auto &tile_layer : (*levelData)["layers"]) {
    if (tile_layer["type"] == "tilelayer") {
      
    
      std::filesystem::path tmj_dir =
          std::filesystem::path(this->tmj_path).parent_path();
      game.entities.push_back(std::make_unique<TilemapEntity>(
          tmj_dir, tile_layer, *levelData, game));
    }
  }
}
