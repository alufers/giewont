#include "LevelLoader.h"
#include "BackdropEntity.h"
#include "FlagEntity.h"
#include "Game.h"
#include "GameplayManager.h"
#include "Log.h"
#include "ParticleSystemEntity.h"
#include "SpawnEntity.h"
#include "TeamBaseEntity.h"
#include "TilemapEntity.h"
#include "entities/gui/MainMenuGUIEntity.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>
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
        auto entity = create_entity_from_json(game, object);
        if (entity != nullptr) {
          game.push_entity(std::move(entity));
        }
      }
    }
  }
}

nlohmann::json
LevelLoader::tmjPropertiesToObj(const nlohmann::json &tmj_properties) {
  nlohmann::json obj;
  for (auto &prop : tmj_properties) {
    if (!prop.contains("name") || !prop["name"].is_string()) {
      throw std::runtime_error("tmjPropertiesToObj: name is not a string");
    }
    obj[prop["name"]] = prop["value"];
  }
  return obj;
}

std::unique_ptr<Entity>
LevelLoader::create_entity_from_json(const Game &game,
                                     const nlohmann::json &object) {
  std::unique_ptr<Entity> entity = nullptr;
  // detect entity type
  if (object["type"] == "spawn_entity") {
    entity = std::make_unique<SpawnEntity>(object);
    entity->is_static = true;

  } else if (object["type"] == "particle_system") {
    entity = std::make_unique<ParticleSystemEntity>(object);
    entity->is_static = true;

  } else if (object["type"] == "backdrop") {
    entity = std::make_unique<BackdropEntity>(object);
    entity->is_static = true;

  } else if (object["type"] == "flag") {
    entity = std::make_unique<FlagEntity>(object);
    entity->is_static = true;
  } else if (object["type"] == "team_base") {
    // Only spawn team bases on the server, they will get synced.
    if (game.is_server()) {
      entity = std::make_unique<TeamBaseEntity>(object);
    }

  } else if (object["type"] == "gameplay_manager") {
    if (game.is_server()) {
      entity = std::make_unique<GameplayManager>();
    }

  } else if (object["type"] == "main_menu_gui") {
    if (!game.is_server()) {
      entity = std::make_unique<MainMenuGUIEntity>();
    }

  } else {
    LOG_ERROR() << "Unknown object type in level data: " << object["type"]
                << std::endl;
  }

  return entity;
}
