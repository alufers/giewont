#pragma once

#include "GoapInterfaces.h"
#include <vector>

namespace giewont {

std::vector<std::unique_ptr<GoapGoal>> construct_goap_goals();
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


}; // namespace giewont
