#pragma once

#include "Entity.h"
#include "GameplayManager.h"
#include <functional>
namespace giewont {

class SmartAIThinkCtx;

typedef std::function<bool(SmartAIThinkCtx &ctx, EntityRef)>
    GoapEntityFilterFunc;
typedef std::function<EntityRef(SmartAIThinkCtx &ctx)> GoapEntitySelectorFunc;

namespace goap_selectors {
bool is_enemy_character(SmartAIThinkCtx &ctx, EntityRef entity_ref);
bool is_friendly_character(SmartAIThinkCtx &ctx, EntityRef entity_ref);
bool is_enemy_flag(SmartAIThinkCtx &ctx, EntityRef entity_ref);
bool is_friendly_flag(SmartAIThinkCtx &ctx, EntityRef entity_ref);
bool is_enemy_base(SmartAIThinkCtx &ctx, EntityRef entity_ref);
bool is_friendly_base(SmartAIThinkCtx &ctx, EntityRef entity_ref);

/// @brief Selects the closest entity that matches the filter
GoapEntitySelectorFunc closest_selector(GoapEntityFilterFunc filter);

/// @brief Select the first matched entity that matches the filter
GoapEntitySelectorFunc first_selector(GoapEntityFilterFunc filter);

}; // namespace goap_selectors

}; // namespace giewont
