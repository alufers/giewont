#pragma once

#include "Entity.h"
#include <Vec2.h>
#include <cmath>
#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <variant>

namespace giewont {

class SmartAIThinkCtx;

enum class GoapBlackboardKey {
  INVALID = 0,

  // @brief Keys about own state
  IS_PROPPED,
  HEALTH_PERCENTAGE,
  IS_HOLDING_ENEMY_FLAG,
  IS_IN_WATER,

  // @brief Keys about enemy state
  IS_ANYBODY_HOLDING_ENEMY_FLAG,
  IS_ENEMY_FLAG_IN_CAPTURED_ANIMATION, // The flag has been captured and is being raised by the base.

  // @brief positions of various points of interest
  OWN_POS,
  ENEMY_FLAG_POS,
  ENEMY_BASE_POS,
  OWN_FLAG_POS,
  OWN_BASE_POS,
  CLOSEST_ENEMY_POS,
  CLOSEST_FRIENDLY_POS,
  CLOSEST_HEALTHKIT_POS,

  // @brief Keys about enemy characters
  HAS_LINE_OF_SIGHT_TO_CLOSEST_ENEMY,
  CLOSEST_ENEMY_HEALTH_PERCENTAGE,

  // @brief Keys about own team state
  IS_ANYBODY_HOLDING_OWN_FLAG,
  OWN_POINTS,

  // @brief Keys about pathfinding and updates
  UPDATES_SINCE_LAST_PATHFINDING,

};

typedef std::variant<bool, float, int, Vec2, EntityRef> GoapBlackboardValue;

std::string goap_blackboard_key_to_string(GoapBlackboardKey key);
std::string goap_blackboard_value_to_string(const GoapBlackboardValue &value);

typedef std::map<GoapBlackboardKey, GoapBlackboardValue> GoapBlackboard;

class GoapSensor {

public:
  virtual ~GoapSensor() = default;

  virtual GoapBlackboardKey get_key() const = 0;
  virtual GoapBlackboardValue sense(SmartAIThinkCtx &ctx) = 0;

  // @brief How often to check the sensor
  uint32_t sense_frequency = 1;
  uint32_t last_update_frame = 0;
};

enum class GoapActionResult {
  NOT_ATTEMPTED, /// @brief The action has not been attempted yet
  IN_PROGRESS, /// @brief The action is still being successfully performed
  DONE,     /// @brief The action has been successfully completed, next plan item can be performed
  FAILED_RECOVERABLE, /// @brief The action has failed, but it can be begun again
  FAILED_FORCE_REPLAN, /// @brief The action has failed and the action plan must be re-calculated
  RESTART_IMMEDIATE, /// @brief Restart the action immediately, without dwelling and counting the action as failed
};

std::string goap_action_result_to_string(GoapActionResult result);

/// @brief Represents one action that the Agent can perform
class GoapAction {
public:
  virtual ~GoapAction() = default;

  /// @brief Compute the cost (negative reward) of the action and the resulting
  /// state
  /// @param ctx The context of the AI thinking
  /// @param initial_state The initial state of the blackboard
  /// @param[out] finish_state_out The state after the action is performed, if
  /// it is possible to perform the action
  /// @return The cost of the action, or NAN if the action cannot be performed
  virtual float get_reward(SmartAIThinkCtx &ctx,
                           const GoapBlackboard &initial_state,
                           GoapBlackboard &finish_state_out) const = 0;

  /// @brief Get the action name
  virtual std::string get_name() const = 0;

  /// @brief Called when the action is selected for execution
  virtual GoapActionResult on_begin(SmartAIThinkCtx &ctx) {
    return GoapActionResult::IN_PROGRESS;
  }

  /// @brief Execute the action
  /// @param ctx The context of the AI thinking
  /// @return true if the actions should be continued, false if the action is
  /// finished
  virtual GoapActionResult perform(SmartAIThinkCtx &ctx) = 0;

  float _last_reward_value = 0.0f; // The last cost value for debugging
};

class GoapGoal {
public:
  virtual ~GoapGoal() = default;

  /// @brief Get the goal name
  virtual std::string get_name() const = 0;

  /// @brief Get the reward given the result state.
  /// @param ctx The context of the AI thinking
  /// @param initial_state The initial state of the blackboard before actions
  /// are performed
  /// @param result_state The state of the blackboard after actions were
  /// performed The goal should preferably only depend on result_state, because
  /// rewards depending on changes between initial_state and result_state might
  /// be harder to debug, since the reward will be applied in one frame.
  virtual float get_reward(SmartAIThinkCtx &ctx,
                           const GoapBlackboard &initial_state,
                           const GoapBlackboard &result_state) const = 0;

  /// @brief The reward based on the current world state is stored here to
  /// display during debugging
  float _last_reward_value = 0.0f;
};

class GoapPlanItem {
public:
  std::weak_ptr<GoapAction> action; // The action to perform
  GoapBlackboard state;             // The state after the action is performed
  float action_reward = 0.0f; // The (negative) reward obtained from the action
  float goal_reward = 0.0f; // The (positive) reward value obtained from judging the state after the action is performed
  uint32_t item_depth = 0;
  bool did_begin = false; // Used to call on_begin() only once for the action
  bool did_complete = false;
  size_t attempt_count = 0;
  GoapActionResult last_result = GoapActionResult::NOT_ATTEMPTED;
};

} // namespace giewont
