#include "LevelLoader.h"
#include "Game.h"
#include "TilemapEntity.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include "SpawnEntity.h"
#include "Log.h"
using namespace giewont;

LevelLoader::LevelLoader(std::string tmj_path) : tmj_path(tmj_path) {}

void LevelLoader::load_level(Game &game) {
  this->level_res_id = game.rm->load_json(this->tmj_path);
  auto levelData = game.rm->get_json(this->level_res_id);

  for (auto &tile_layer : (*levelData)["layers"]) {
    if (tile_layer["type"] == "tilelayer") {

      std::filesystem::path tmj_dir =
          std::filesystem::path(this->tmj_path).parent_path();
      auto tilemap = std::make_unique<TilemapEntity>(tmj_dir, tile_layer,
                                                     *levelData, game);
      tilemap->is_static = true;
      game.push_entity(std::move(tilemap));
    } else if (tile_layer["type"] == "objectgroup") {
      LOG_DEBUG() << "Object group" << std::endl;
      for (auto &object : tile_layer["objects"]) {
        
        // detect entity type
        if (object["type"] == "spawn_entity") {
          auto spawn = std::make_unique<SpawnEntity>(object);
          spawn->is_static = true;
          game.push_entity(std::move(spawn));
        }
      }
    }
  }
}
