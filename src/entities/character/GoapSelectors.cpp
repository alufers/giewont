#include "GoapSelectors.h"
#include "FlagEntity.h"
#include "SmartAIContainers.h"
#include "TeamBaseEntity.h"
using namespace giewont;

GoapEntitySelectorFunc
goap_selectors::closest_selector(GoapEntityFilterFunc filter) {
  return [filter](SmartAIThinkCtx &ctx) -> EntityRef {
    EntityRef closest_entity;
    float closest_distance = std::numeric_limits<float>::max();

    for (const auto &entity : ctx.game.valid_entities()) {
      if (filter(ctx, entity->get_ref())) {
        float distance = (ctx.character.position - entity->position).length();
        if (distance < closest_distance) {
          closest_distance = distance;
          closest_entity = entity->get_ref();
        }
      }
    }

    return closest_entity;
  };
}

GoapEntitySelectorFunc
goap_selectors::first_selector(GoapEntityFilterFunc filter) {
  return [filter](SmartAIThinkCtx &ctx) -> EntityRef {
    for (const auto &entity : ctx.game.valid_entities()) {
      if (filter(ctx, entity->get_ref())) {
        return entity->get_ref();
      }
    }
    return EntityRef();
  };
}

bool goap_selectors::is_enemy_character(SmartAIThinkCtx &ctx,
                                        EntityRef entity_ref) {
  if (entity_ref.valid_as<CharacterEntity>(ctx.game)) {
    auto &character = entity_ref.get_as<CharacterEntity>(ctx.game);
    return character.team != ctx.character.team;
  }

  return false;
}

bool goap_selectors::is_friendly_character(SmartAIThinkCtx &ctx,
                                           EntityRef entity_ref) {
  if (entity_ref.valid_as<CharacterEntity>(ctx.game)) {
    auto &character = entity_ref.get_as<CharacterEntity>(ctx.game);
    return character.team == ctx.character.team;
  }

  return false;
}

bool goap_selectors::is_enemy_flag(SmartAIThinkCtx &ctx, EntityRef entity_ref) {
  if (entity_ref.valid_as<FlagEntity>(ctx.game)) {
    auto &flag = entity_ref.get_as<FlagEntity>(ctx.game);
    return flag.team != ctx.character.team;
  }

  return false;
}

bool goap_selectors::is_friendly_flag(SmartAIThinkCtx &ctx,
                                      EntityRef entity_ref) {
  if (entity_ref.valid_as<FlagEntity>(ctx.game)) {
    auto &flag = entity_ref.get_as<FlagEntity>(ctx.game);
    return flag.team == ctx.character.team;
  }

  return false;
}

bool goap_selectors::is_enemy_base(SmartAIThinkCtx &ctx, EntityRef entity_ref) {
  if (entity_ref.valid_as<TeamBaseEntity>(ctx.game)) {
    auto &base = entity_ref.get_as<TeamBaseEntity>(ctx.game);
    return base.team != ctx.character.team;
  }

  return false;
}

bool goap_selectors::is_friendly_base(SmartAIThinkCtx &ctx,
                                      EntityRef entity_ref) {
  if (entity_ref.valid_as<TeamBaseEntity>(ctx.game)) {
    auto &base = entity_ref.get_as<TeamBaseEntity>(ctx.game);
    return base.team == ctx.character.team;
  }

  return false;
}
