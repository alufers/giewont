#include "GoapSelectors.h"
#include "FlagEntity.h"
#include "SmartAIContainers.h"
#include "TeamBaseEntity.h"
#include "entities/gameplay/BonusEntity.h"

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

bool goap_selectors::is_valid_entity(SmartAIThinkCtx &ctx,
                                     EntityRef entity_ref) {
  return entity_ref.valid(ctx.game);
}

bool goap_selectors::is_healthkit(SmartAIThinkCtx &ctx, EntityRef entity_ref) {
  if (entity_ref.valid_as<BonusEntity>(ctx.game)) {
    auto &bonus = entity_ref.get_as<BonusEntity>(ctx.game);
    return bonus.bonus_type == net::BonusEntityType::HEALTHKIT;
  }
  return false;
}

bool goap_selectors::is_self(SmartAIThinkCtx &ctx, EntityRef entity_ref) {
  return entity_ref == ctx.character.get_ref();
}

GoapEntityFilterFunc
goap_selectors::negate_filter(const GoapEntityFilterFunc filter) {
  return [filter](SmartAIThinkCtx &ctx, EntityRef entity_ref) {
    return !filter(ctx, entity_ref);
  };
}

GoapEntityFilterFunc goap_selectors::and_filter(const GoapEntityFilterFunc a,
                                                const GoapEntityFilterFunc b) {
  return [a, b](SmartAIThinkCtx &ctx, EntityRef entity_ref) {
    return a(ctx, entity_ref) && b(ctx, entity_ref);
  };
}
