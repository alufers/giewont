#include "Game.h"
#include "CameraEntity.h"
#include "Log.h"
#include "NullEntity.h"
#include <exception>
#include <format>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "LevelLoader.h"

using namespace giewont;

Game::Game() { this->entities.push_back(std::make_unique<NullEntity>()); }

void Game::draw() {
  auto &camera = camera_ref.get_as<CameraEntity>(*this);
  camera.begin_mode2d();
  for (auto &entity : entities) {
    if (entity != nullptr) {
      entity->draw(*this);
    }
  }

  if (debug_overlay) {
    for (auto &entity : entities) {
      if (entity != nullptr) {
        entity->draw_debug(*this);
      }
    }
  }

  camera.end_mode2d();

  DrawText(std::format("UPS: {:.2f}", last_ups).c_str(), 10, 10, 20, BLACK);
}

void Game::update(float delta_time) {
  destroy_marked_entities();

  if (!this->camera_ref.valid(*this)) {
    LOG_WARN() << "Camera not set, creating a new one" << std::endl;
    auto camera = std::make_unique<CameraEntity>();
    this->camera_ref = this->push_entity(std::move(camera));
  }

  for (auto &entity : entities) {
    if (entity != nullptr) {
      entity->update(*this, delta_time);
    }
  }

  last_ups = 1.0 / delta_time;
}

EntityRef Game::push_entity(std::unique_ptr<Entity> entity) {
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

  return entities[idx]->get_ref();
}

void Game::load_assets() {

  LevelLoader level_loader("level1.tmj");
  level_loader.load_level(*this);

  for (auto &entity : entities) {
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
