#include "SpawnEntity.h"
#include "CharacterEntity.h"
#include "Log.h"
#include <exception>

using namespace giewont;

SpawnEntity::SpawnEntity(const nlohmann::json &data) : Entity() {
  LOG_DEBUG() << "SpawnEntity constructor" << std::endl;
  // check if has "properties" key
  if (data.find("properties") != data.end()) {
    auto properties = data["properties"];
    for (auto &property : properties) {
      if (property["name"] == "entity_type") {
        this->entity_type = property["value"];
      }
    }
  } else {
    throw std::runtime_error("SpawnEntity must have a 'properties' key.");
  }

  this->position = Vec2(data["x"], data["y"]);
}

void SpawnEntity::load_assets(const Game &game) {
  // Preload assets for the entity that will be spawned
  auto entity = construct_entity(game);

  if (entity != nullptr) {
    entity->load_assets(game);
  }
}

void SpawnEntity::update(Game &game, float delta_time) {

  if (!did_spawn) {
    did_spawn = true;
    auto entity = construct_entity(game);

    if (entity != nullptr) {
      game.entities.push_back(std::move(entity));
    }
  }
}

void SpawnEntity::draw(const Game &game) {}

std::unique_ptr<Entity> SpawnEntity::construct_entity(const Game &game) {
  if (entity_type == "player_character") {
    auto player = std::make_unique<CharacterEntity>();
    player->position = this->position;
    player->load_assets(game);
    return std::move(player);
  }

  LOG_ERROR() << "Unknown entity type: " << entity_type << std::endl;

  return nullptr;
}
