#pragma once

#include "GoapInterfaces.h"
#include "GoapSelectors.h"

namespace giewont {
class GoToEntityGoapAction : public GoapAction {
public:
  GoToEntityGoapAction(std::string name, GoapEntitySelectorFunc target_entity);

  std::string get_name() const override;
  float get_cost(SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
                 GoapBlackboard &finish_state_out) const override;

  bool perform(SmartAIThinkCtx &ctx) override;

private:
  std::string _name;
  GoapEntitySelectorFunc _target_entity;
};

} // namespace giewont
