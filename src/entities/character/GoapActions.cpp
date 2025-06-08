#include "GoapActions.h"
#include "SmartAIContainers.h"

using namespace giewont;

GoToEntityGoapAction::GoToEntityGoapAction(std::string name,
                                           GoapEntitySelectorFunc target_entity)
    : _name(std::move(name)), _target_entity(target_entity) {}

std::string GoToEntityGoapAction::get_name() const { return _name; }

float GoToEntityGoapAction::get_cost(SmartAIThinkCtx &ctx,
                                     const GoapBlackboard &initial_state,
                                     GoapBlackboard &finish_state_out) const {
  EntityRef target_entity = _target_entity(ctx);
  if (!target_entity.valid(ctx.game)) {
    return INFINITY;
  }

    Vec2 target_pos = target_entity.get(ctx.game).position;
    
}
