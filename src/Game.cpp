#include "Game.h"
#include "CameraEntity.h"
#include "Entity.h"
#include "Log.h"
#include "NullEntity.h"
#include <exception>

#include "CharacterEntity.h"
#include "LevelLoader.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace giewont;

Game::Game() { this->entities.push_back(std::make_unique<NullEntity>()); }

void Game::update(float delta_time) {
  destroy_marked_entities();

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

void Game::apply_sync_entity(const net::SyncEntityNetMessage::Reader &message) {
  EntityRef ref;

  for (auto &entity : entities) {
    if (entity != nullptr && entity->net_id == message.getNetId()) {
      ref = entity->get_ref();
    }
  }

  if (!ref.valid(*this)) {
    // Entity not found

    if (this->is_server()) {
      LOG_WARN() << "apply_sync_entity: Entity with net_id "
                 << message.getNetId() << " not found, ignoring" << std::endl;
      return;
    } else {

      std::unique_ptr<Entity> entToCreate;

      switch (message.getEntityType()) {
      case net::EntityType::CHARACTER:
        entToCreate = std::make_unique<CharacterEntity>();
        break;
      default:
        LOG_WARN() << "apply_sync_entity: Unknown entity type" << std::endl;
        return;
      }

      entToCreate->net_id = message.getNetId();
      entToCreate->is_being_created = true;
      if (!this->is_server()) {
        entToCreate->net_owner_peer_id = message.getNetOwnerId();
      }

      entToCreate->load_assets(*this);

      ref = this->push_entity(std::move(entToCreate));
    }
  }

  if (!this->is_server()) {
    ref.get(*this).net_owner_peer_id = message.getNetOwnerId();
  }

  ref.get(*this).update_from_sync_message(*this, message);
  ref.get(*this).is_being_created = false;
}

void Game::load_level(std::string tmj_path) {
  LevelLoader level_loader(tmj_path);
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

EntityRef Game::get_entity_by_net_id(uint32_t net_id) {
  for (auto &entity : entities) {
    if (entity != nullptr && entity->net_id == net_id) {
      return entity->get_ref();
    }
  }
  LOG_WARN() << "get_entity_by_net_id: Entity with net_id " << net_id
             << " not found" << std::endl;
  return EntityRef();
}
