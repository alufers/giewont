#pragma once

#include "GoapInterfaces.h"
#include "GoapSelectors.h"
#include "GoapGoToEntityAction.h"
#include <vector>

namespace giewont {



std::vector<std::shared_ptr<GoapAction>> construct_goap_actions();



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

  GoapActionResult perform(SmartAIThinkCtx &ctx) override;
};

/**
 * @brief An action which uses the interaction system to pick up a healthkit.
 */
class PickupHealthkit : public GoapAction {
public:
  PickupHealthkit();

  std::string get_name() const override;

  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                 GoapBlackboard &finish_state_out) const override;

  GoapActionResult perform(SmartAIThinkCtx &ctx) override;
};



class DwellAtOwnBaseWaitingForFlagToBeCaptured : public GoapAction {
public:
  DwellAtOwnBaseWaitingForFlagToBeCaptured();

  std::string get_name() const override;

  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                 GoapBlackboard &finish_state_out) const override;

  GoapActionResult perform(SmartAIThinkCtx &ctx) override;
};

} // namespace giewont
