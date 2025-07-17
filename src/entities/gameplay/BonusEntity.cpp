#include "BonusEntity.h"
#include "GameplayManager.h"
#include "entities/character/CharacterEntity.h"

using namespace giewont;

BonusEntity::BonusEntity() : PhysEntity() {
  this->mass = 1000.0f; // Large mass
}

void BonusEntity::load_assets(const Game &game) {
  PhysEntity::load_assets(game);
  bonus_aabb = AABB(Vec2(0, 0), Vec2(70, 70));
  _texture_id = game.rm->load_texture("entities/healthkit.png");
  _destroy_particle_res_id =
      game.preload_prefab("prefabs/character_fall_small.json");
}

void BonusEntity::update(Game &game, float delta_time) {
  PhysEntity::update(game, delta_time);

  if (game.is_server()) {
    lifetime -= delta_time;
    if (lifetime <= 0.0f) {
      this->destroy();
    }
  }
}

void BonusEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_id);
  Color tint = WHITE;

  float scale = 1.0f + (std::sin(lifetime * 4.0f) * 0.05f);

  DrawTextureEx(*tex, position.to_raylib(), 0.0f, scale, tint);
#endif
}

void BonusEntity::build_sync_message(
    Game &game, net::SyncEntityNetMessage::Builder &sync_message) {
  PhysEntity::build_sync_message(game, sync_message);
  auto bonusData = sync_message.initExtraData().initBonusData();
  bonusData.setTotalLifetime(total_lifetime);
  bonusData.setLifetime(lifetime);
  bonusData.setBonusType(bonus_type);
}

void BonusEntity::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  PhysEntity::update_from_sync_message(game, sync_message);
  auto bonusData = sync_message.getExtraData().getBonusData();
  total_lifetime = bonusData.getTotalLifetime();
  lifetime = bonusData.getLifetime();
  bonus_type = bonusData.getBonusType();
}

net::EntityType BonusEntity::get_net_type() { return net::EntityType::BONUS; }

void BonusEntity::draw_raylib_ui(const Game &game) {}

bool BonusEntity::handle_interaction(
    Game &game, const net::InteractNetMessage::Reader interaction) {
  if (interaction.getType() != net::InteractionType::USE_INTERACTION) {
    return false;
  }

  EntityRef interactor =
      game.get_entity_by_net_id(interaction.getInteractorNetId());
  if (!interactor.valid_as<CharacterEntity>(game)) {
    return false;
  }

  auto dist = position.distance(interactor.get(game).position);

  if (dist > game.get_gvar<float>(GVarType::ENTITY_INTERACTION_RANGE)) {
    return false;
  }

  auto &character = interactor.get_as<CharacterEntity>(game);
  character.hurt_entity(game, -1 * (int)amount);
  this->destroy();
  return true;
}

bool BonusEntity::check_interaction_possible(Game &game, EntityRef interactor) {
  if (!interactor.valid_as<CharacterEntity>(game)) {
    return false;
  }

  auto &character = interactor.get_as<CharacterEntity>(game);
  auto dist = position.distance(character.position);

  return dist <= game.get_gvar<float>(GVarType::ENTITY_INTERACTION_RANGE);
}
