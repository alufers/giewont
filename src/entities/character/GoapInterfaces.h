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

  // @brief Keys about enemy state
  DIST_TO_ENEMY_FLAG,
  DIST_TO_ENEMY_BASE,
  IS_ANYBODY_HOLDING_ENEMY_FLAG,
  ENEMY_FLAG_DIST_TO_OWN_BASE,
  DIST_BETWEEN_BASES,

  // @brief Keys about other characters
  DIST_TO_CLOSEST_ENEMY,
  DIST_TO_CLOSEST_FRIENDLY,

  // @brief Keys about own team state
  DIST_TO_OWN_FLAG,
  DIST_TO_OWN_BASE,
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

/// @brief Represents one action that the Agent can perform
class GoapAction {
public:
  virtual ~GoapAction() = default;

  /// @brief Compute the cost of the action and the resulting state
  /// @param ctx The context of the AI thinking
  /// @param initial_state The initial state of the blackboard
  /// @param[out] finish_state_out The state after the action is performed, if
  /// it is possible to perform the action
  /// @return The cost of the action, or NAN if the action cannot be performed
  virtual float get_cost(SmartAIThinkCtx &ctx,
                         const GoapBlackboard &initial_state,
                         GoapBlackboard &finish_state_out) const = 0;

  /// @brief Get the action name
  virtual std::string get_name() const = 0;

  /// @brief Called when the action is selected for execution
  virtual void on_begin(SmartAIThinkCtx &ctx) {}

  /// @brief Execute the action
  /// @param ctx The context of the AI thinking
  /// @return true if the actions should be continued, false if the action is
  /// finished
  virtual bool perform(SmartAIThinkCtx &ctx) = 0;
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

} // namespace giewont
