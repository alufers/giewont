#include "GoapGoals.h"
#include "SmartAIContainers.h"

#include <cmath>

using namespace giewont;

std::vector<std::unique_ptr<GoapGoal>> giewont::construct_goap_goals() {
    std::vector<std::unique_ptr<GoapGoal>> goals;
    
    goals.push_back(std::make_unique<BringEnemyFlagToBaseGoal>());
    
   
    return goals;
}

float BringEnemyFlagToBaseGoal::get_reward(
    SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
    const GoapBlackboard &result_state) const {

  float flag_dist_to_own_base = std::get<float>(
      result_state.at(GoapBlackboardKey::ENEMY_FLAG_DIST_TO_OWN_BASE));

  float dist_between_bases =
      std::get<float>(result_state.at(GoapBlackboardKey::DIST_BETWEEN_BASES));

  float base_activation_dist =
      ctx.game.get_gvar<float>(GVarType::BASE_ACTIVATION_DIST);

  float dist_to_reach = dist_between_bases - base_activation_dist;

  if (!std::isfinite(flag_dist_to_own_base) || !std::isfinite(dist_to_reach) ||
      dist_to_reach <= 0.0f) {
    return 0.0f; // Invalid state, no reward
  }

  auto percentage_reached =
      1 - std::clamp(flag_dist_to_own_base / dist_to_reach, 0.0f, 1.0f);

  return total_reward * percentage_reached;
}
