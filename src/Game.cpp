#include "Game.h"
#include "CameraEntity.h"
#include "Entity.h"
#include "FlagEntity.h"
#include "GameplayManager.h"
#include "Log.h"
#include "NullEntity.h"
#include <exception>

#include "CharacterEntity.h"
#include "GrenadeEntity.h"
#include "LevelLoader.h"
#include "TeamBaseEntity.h"
#include "Vec2.h"
#include "entities/decorative/TombstoneEntity.h"
#include "entities/gui/TeamChoiceGUIEntity.h"
#include <cstdint>
#include <format>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace giewont;

Game::Game() {
  auto null_ent = std::make_unique<NullEntity>();
  null_ent->id = 0;
  this->entities.push_back(std::move(null_ent));
  this->entities.resize(MAX_ENTITIES);
}

void Game::update(float delta_time) {
  delete_marked_entities();

  for (auto &entity : entities) {
    if (entity != nullptr) {
      entity->update(*this, delta_time);
    }
  }

  last_ups = 1.0 / delta_time;
}

EntityRef Game::push_entity(std::unique_ptr<Entity> entity) {
  int64_t idx = -1;
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
  LOG_DEBUG() << "Pushing entity with id " << idx << " (type "
              << entity->get_type_name() << ")" << std::endl;
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
      case net::EntityType::FLAG:
        entToCreate = std::make_unique<FlagEntity>();
        break;
      case net::EntityType::TEAM_BASE:
        entToCreate = std::make_unique<TeamBaseEntity>();
        break;
      case net::EntityType::GAMEPLAY_MANAGER:
        entToCreate = std::make_unique<GameplayManager>();
        break;
      case net::EntityType::GRENADE:
        entToCreate = std::make_unique<GrenadeEntity>();
        break;
      case net::EntityType::TEAM_CHOICE_G_U_I:
        entToCreate = std::make_unique<TeamChoiceGUIEntity>();
        break;
      case net::EntityType::TOMBSTONE:
        entToCreate = std::make_unique<TombstoneEntity>();
        break;
      default:
        LOG_WARN() << "apply_sync_entity: Unknown entity type "
                   << (int)message.getEntityType() << ", ignoring" << std::endl;
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
  for (auto &entity : entities) {
    if (entity != nullptr && !entity->marked_for_deletion && entity->id != 0) {
      entity->destroy();
    }
  }
  delete_marked_entities();
  LevelLoader level_loader(tmj_path);
  level_loader.load_level(*this);

  for (auto &entity : entities) {
    if (entity != nullptr) {
      entity->load_assets(*this);
    }
  }
}

EntityRef Game::get_entity_by_net_id(uint32_t net_id) {
  if (net_id == 0) {
    return EntityRef();
  }
  for (auto &entity : entities) {
    if (entity != nullptr && entity->net_id == net_id) {
      return entity->get_ref();
    }
  }
  LOG_WARN() << "get_entity_by_net_id: Entity with net_id " << net_id
             << " not found" << std::endl;
  return EntityRef();
}

res_id Game::preload_prefab(std::string prefab_path) const {
  res_id id = this->rm->load_json(prefab_path);
  auto ent =
      LevelLoader::create_entity_from_json(*this, *this->rm->get_json(id));
  if (ent == nullptr) {
    throw std::runtime_error(
        std::format("Failed to create entity from prefab: {}", prefab_path));
  }
  ent->load_assets(*this);
  return id;
}

EntityRef Game::instantiate_prefab(res_id prefab_res) {
  auto ent = LevelLoader::create_entity_from_json(
      *this, *this->rm->get_json(prefab_res));
  if (ent == nullptr) {
    throw std::runtime_error(
        std::format("Failed to create entity from prefab: {}", prefab_res));
  }
  ent->load_assets(*this);

  return this->push_entity(std::move(ent));
}

EntityRef Game::instantiate_prefab(res_id prefab_res, Vec2 pos) {
  EntityRef ref = instantiate_prefab(prefab_res);

  ref.get(*this).position = pos;

  return ref;
}

void Game::delete_marked_entities() {

  for (auto &entity : entities) {
    if (entity == nullptr)
      continue;
    bool is_server_or_owns_entity =
        this->is_server() || (entity->net_owner_peer_id == my_peer_id);
    if (entity->marked_for_deletion && !entity->is_static &&
        entity->net_id != 0 && is_server_or_owns_entity) {
      LOG_INFO()
          << "Notifying about the destruction of entity entity  with net_id="
          << entity->net_id << " (type: " << entity->get_type_name() << ")"
          << std::endl;

      ::capnp::MallocMessageBuilder message;
      auto root = message.initRoot<net::BaseNetMessage>();
      auto destroyEntity = root.initDestroyEntity();
      destroyEntity.setNetId(entity->net_id);

      broadcast_reliable(message);
    }
  }

  for (size_t i = 0; i < entities.size(); i++) {
    if (entities[i] != nullptr && entities[i]->marked_for_deletion) {
      entities[i] = nullptr;
    }
  }
}
