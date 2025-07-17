#pragma once

#include "GoapInterfaces.h"
#include <vector>

namespace giewont {

std::vector<std::unique_ptr<GoapGoal>> construct_goap_goals();
std::vector<std::unique_ptr<GoapGoal>> construct_killer_goap_goals();
std::vector<std::unique_ptr<GoapGoal>> construct_protector_goap_goals();

class BringEnemyFlagToBaseGoal : public GoapGoal {
public:
  float total_reward = 10000.0f;
  std::string get_name() const override { return "BringEnemyFlagToBaseGoal"; }
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   const GoapBlackboard &result_state) const override;
};


class KeepHealthAboveHalf : public GoapGoal {
public:
  float total_reward = 20000.0f;
  std::string get_name() const override { return "KeepHealthAboveHalf"; }
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   const GoapBlackboard &result_state) const override;
};


class FollowTeammateWithFlag : public GoapGoal {
public:
  float total_reward = 30000.0f;
  std::string get_name() const override { return "FollowTeammateWithFlag"; }
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   const GoapBlackboard &result_state) const override;
};

class KeepEnemiesAwayFromSelfGoal : public GoapGoal {
public:
  float total_reward = 90000.0f;
  std::string get_name() const override { return "KeepEnemiesAwayFromSelfGoal"; }
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   const GoapBlackboard &result_state) const override;
};

class KillEnemiesGoapGoal : public GoapGoal {
public:
  float total_reward = 90000.0f;
  std::string get_name() const override { return "KillEnemiesGoapGoal"; }
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   const GoapBlackboard &result_state) const override;
};

class StayNearOwnFlagGoapGoal : public GoapGoal {
public:
  float total_reward = 100000.0f;
  std::string get_name() const override { return "StayNearOwnFlagGoapGoal"; }
  float get_reward(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                   const GoapBlackboard &result_state) const override;
};



}; // namespace giewont
