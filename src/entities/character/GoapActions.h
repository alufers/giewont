#pragma once

#include "GoapGoToEntityAction.h"
#include "GoapInterfaces.h"
#include "GoapSelectors.h"
#include <vector>

namespace giewont {

std::vector<std::shared_ptr<GoapAction>> construct_goap_actions();

class GoToEnemyFlagGoapAction : public GoToEntityGoapAction {
public:
  GoToEnemyFlagGoapAction();
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   GoapBlackboard &finish_state_out) const override;
};


class GoToClosestEnemyGoapAction : public GoToEntityGoapAction {
public:
  GoToClosestEnemyGoapAction();
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   GoapBlackboard &finish_state_out) const override;
  GoapActionResult on_begin(SmartAIThinkCtx &ctx) override ;
  GoapActionResult perform(SmartAIThinkCtx &ctx) override;
};

/**
 * @brief Pathfinds to the enemy flag, and if it moves it re-pathfinds without
 * any delays.
 */
class FollowEnemyFlagAction : public GoToEntityGoapAction {
public:
  FollowEnemyFlagAction();
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   GoapBlackboard &finish_state_out) const override;
};


/**
 * @brief Pathfinds own flag, and if it moves it re-pathfinds without
 * any delays.
 */
class FollowOwnFlagAction : public GoToEntityGoapAction {
public:
  FollowOwnFlagAction();
  
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
 * @brief Drops the enemy flag if the character is holding it.
 */
class DropEnemyFlag : public GoapAction {
public:
  DropEnemyFlag();

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

/**
 * @brief An action which makes the character jump and move left, used for
 * exiting water.
 */
class JumpAndMoveLeft : public GoapAction {
public:
  JumpAndMoveLeft();

  std::string get_name() const override;

  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   GoapBlackboard &finish_state_out) const override;

  GoapActionResult perform(SmartAIThinkCtx &ctx) override;
};

class ShootAtClosestEnemy : public GoapAction {
public:
  ShootAtClosestEnemy();
  std::string get_name() const override;
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   GoapBlackboard &finish_state_out) const override;
  GoapActionResult perform(SmartAIThinkCtx &ctx) override;
};

} // namespace giewont
