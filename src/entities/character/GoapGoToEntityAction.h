#pragma once

#include "GoapInterfaces.h"
#include "GoapSelectors.h"
#include "SmartAIContainers.h"
#include <vector>

namespace giewont {

/**
 * @brief A GOAP action which pathfinds to a target entity and then moves
 * towards it.
 *
 */
class GoToEntityGoapAction : public GoapAction {
public:
  /**
   * @brief Construct a new Go To Entity Goap Action object
   *
   * @param name Name of the action for debugging
   * @param target_entity The entity selector used to actually go to the entity.
   * @param target_pos_key If provided, then the position from this blackboard
   * key is used for estimating the cost, instead of the current position of the
   * entity.
   */
  GoToEntityGoapAction(
      std::string name, GoapEntitySelectorFunc target_entity,
      GoapBlackboardKey target_pos_key = GoapBlackboardKey::INVALID);

  std::string get_name() const override;

  /**
   * @brief Returns the cost of the movement to the entity and the predicted
   * state after the action is completed. Takes into account changing the
   * position of the flag, if the character is holding it.
   */
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   GoapBlackboard &finish_state_out) const override;

  GoapActionResult on_begin(SmartAIThinkCtx &ctx) override;

  GoapActionResult perform(SmartAIThinkCtx &ctx) override;

protected:

  bool allow_target_entity_movement = false; // If true movement of the target entity will not be considered as a pathfinding failure

private:
  std::string _name;
  GoapEntitySelectorFunc _target_entity;
  GoapBlackboardKey _target_pos_key;

  /// @brief Find a path to the target position using A* algorithm
  void find_path(SmartAIThinkCtx &ctx, Vec2 target_pos);

  /// @brief Fill in state.path with the path from start to goal given a goal
  /// node
  void reconstruct_path(SmartAIThinkCtx &ctx, AiPathNode *goalNode);
};

}; // namespace giewont
