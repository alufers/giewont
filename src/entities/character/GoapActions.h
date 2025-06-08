#pragma once

#include "GoapInterfaces.h"
#include "GoapSelectors.h"
#include <vector>

namespace giewont {



std::vector<std::shared_ptr<GoapAction>> construct_goap_actions();


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

  void on_begin(SmartAIThinkCtx &ctx) override;

  bool perform(SmartAIThinkCtx &ctx) override;

private:
  std::string _name;
  GoapEntitySelectorFunc _target_entity;
  GoapBlackboardKey _target_pos_key;
};

class GoToEnemyFlagGoapAction : public GoToEntityGoapAction {
public:
  GoToEnemyFlagGoapAction();
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                 GoapBlackboard &finish_state_out) const override;
};

/**
 * @brief An action which uses the interaction system to pick up the enemy
 * flag, if the character is close enough to it.
 */
class PickUpEnemyFlag : public GoapAction {
public:
  PickUpEnemyFlag();

  std::string get_name() const override;

  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                 GoapBlackboard &finish_state_out) const override;

  bool perform(SmartAIThinkCtx &ctx) override;
};


class DwellAtOwnBaseWaitingForFlagToBeCaptured : public GoapAction {
public:
  DwellAtOwnBaseWaitingForFlagToBeCaptured();

  std::string get_name() const override;

  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                 GoapBlackboard &finish_state_out) const override;

  bool perform(SmartAIThinkCtx &ctx) override;
};

} // namespace giewont
