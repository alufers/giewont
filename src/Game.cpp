#include "Game.h"
#include <exception>
#include <stdexcept>

using giewont::Game;

void Game::draw() {}

void Game::update(float delta_time) { destroy_marked_entities(); }

void Game::push_entity(std::unique_ptr<Entity> entity) {
  ssize_t idx = -1;
  for (size_t i = 0; i < entities.size(); i++) {
    if (entities[i] == nullptr) {
      idx = i;
      break;
    }
  }
  if (idx == -1) {
    if (entities.size() < MAX_ENTITIES) {
      entities.push_back(nullptr);
      idx = entities.size() - 1;
    } else {
      throw std::runtime_error("MAX_ENTITIES exceeded");
    }
  }
  entity->id = idx;
  entity->generation = generation_counter;
  generation_counter++;
  entities[idx] = std::move(entity);
}

void Game::load_assets() {
  for(auto &entity : entities) {
    if (entity != nullptr) {
      entity->load_assets(*this);
    }
  }
}

void Game::destroy_marked_entities() {
  for (size_t i = 0; i < entities.size(); i++) {
    if (entities[i] != nullptr && entities[i]->marked_for_deletion) {
      entities[i] = nullptr;
    }
  }
}
