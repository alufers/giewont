#include "GoapGoals.h"
#include "SmartAIContainers.h"

#include "Log.h"
#include <cmath>
using namespace giewont;

std::vector<std::unique_ptr<GoapGoal>> giewont::construct_goap_goals() {
  std::vector<std::unique_ptr<GoapGoal>> goals;

  goals.push_back(std::make_unique<BringEnemyFlagToBaseGoal>());
  goals.push_back(std::make_unique<KeepHealthAboveHalf>());
  goals.push_back(std::make_unique<FollowTeammateWithFlag>());
   goals.push_back(std::make_unique<KeepEnemiesAwayFromSelfGoal>());


  return goals;
}

float BringEnemyFlagToBaseGoal::get_reward(
    SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
    const GoapBlackboard &result_state) const {

  bool is_holding_enemy_flag =
      std::get<bool>(result_state.at(GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG));
  if (!is_holding_enemy_flag) {
    return 0.0f; // No reward if we are not holding the enemy flag
  }
  Vec2 own_base_pos =
      std::get<Vec2>(result_state.at(GoapBlackboardKey::OWN_BASE_POS));
  Vec2 enemy_flag_pos =
      std::get<Vec2>(result_state.at(GoapBlackboardKey::ENEMY_FLAG_POS));
  Vec2 enemy_base_pos =
      std::get<Vec2>(result_state.at(GoapBlackboardKey::ENEMY_BASE_POS));

  if (!own_base_pos.isfinite() || !enemy_flag_pos.isfinite() ||
      !enemy_base_pos.isfinite()) {
    return 0.0f; // Invalid state, no reward
  }

  float flag_dist_to_own_base = enemy_flag_pos.distance(own_base_pos);

  float dist_between_bases = enemy_base_pos.distance(own_base_pos);

  float base_activation_dist =
      ctx.game.get_gvar<float>(GVarType::BASE_ACTIVATION_DIST);

  float dist_to_reach = dist_between_bases - base_activation_dist;

  if (dist_to_reach <= 0.0f) {
    // Bases are too close, no reward for reaching the flag
    return 0.0f;
  }

  auto percentage_reached =
      1 - std::clamp(flag_dist_to_own_base / dist_to_reach, 0.0f, 1.0f);

  float extra_reward = 0.0f;

  if (std::get<bool>(result_state.at(
          GoapBlackboardKey::IS_ENEMY_FLAG_IN_CAPTURED_ANIMATION))) {
    // If the flag is in the captured animation state, we get extra reward
    extra_reward = 20.0f; // MAGICNUMBER
  }

  return total_reward * percentage_reached + extra_reward;
}

float KeepHealthAboveHalf::get_reward(
    SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
    const GoapBlackboard &result_state) const {

  float health_percentage =
      std::get<float>(result_state.at(GoapBlackboardKey::HEALTH_PERCENTAGE));
  float percentage_reached = std::clamp(health_percentage, 0.0f, 1.0f);
  return total_reward * percentage_reached;
}

float FollowTeammateWithFlag::get_reward(
    SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
    const GoapBlackboard &result_state) const {

  bool is_holding_enemy_flag =
      std::get<bool>(result_state.at(GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG));
  bool is_anybody_holding_enemy_flag = std::get<bool>(
      result_state.at(GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG));

  // No reward if enemy flag is on the floor, or we are holding it
  if (!is_anybody_holding_enemy_flag || is_holding_enemy_flag)
    return 0.0f;

  float dist_to_enemy_flag = giewont::blackboard_pos_distance(
      result_state, GoapBlackboardKey::ENEMY_FLAG_POS,
      GoapBlackboardKey::OWN_POS);
  if (!std::isfinite(dist_to_enemy_flag))
    return 0.0f; // Invalid state, no reward

  float target_dist =
      300.0f; // MAGICNUMBER, distance to the teammate with the flag

  float percentage_reached =
      std::clamp((target_dist - dist_to_enemy_flag) / target_dist, 0.0f, 1.0f);

  return total_reward * percentage_reached;
}


float KeepEnemiesAwayFromSelfGoal::get_reward(
    SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
    const GoapBlackboard &result_state) const {

  Vec2 own_pos = std::get<Vec2>(result_state.at(GoapBlackboardKey::OWN_POS));
  Vec2 closest_enemy_pos =
      std::get<Vec2>(result_state.at(GoapBlackboardKey::CLOSEST_ENEMY_POS));

  if (!own_pos.isfinite() ) {
    return 0.0f; // Invalid state, no reward
  }

  if(!closest_enemy_pos.isfinite()) {
    return total_reward; // No enemies, we are safe, return full reward
  }

  float dist_to_closest_enemy = own_pos.distance(closest_enemy_pos);
  float target_dist = 80.0f * 9.0f; // MAGICNUMBER, distance to keep from the enemy
  

  float percentage_reached = std::clamp(dist_to_closest_enemy / target_dist,
                                        0.0f, 1.0f); // MAGICNUMBER

  return total_reward * percentage_reached;
}
