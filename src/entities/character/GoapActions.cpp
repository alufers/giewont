#include "GoapActions.h"
#include "Log.h"
#include "SmartAIContainers.h"
using namespace giewont;

std::vector<std::shared_ptr<GoapAction>> giewont::construct_goap_actions() {
  std::vector<std::shared_ptr<GoapAction>> actions;
  actions.emplace_back(std::make_shared<GoToEnemyFlagGoapAction>());
  actions.emplace_back(std::make_shared<PickUpEnemyFlag>());
  actions.emplace_back(std::make_shared<GoToEntityGoapAction>(
      "GoToOwnBase",
      goap_selectors::closest_selector(goap_selectors::is_friendly_base),
      GoapBlackboardKey::OWN_BASE_POS));

  actions.emplace_back(std::make_shared<GoToEntityGoapAction>(
      "GoToClosestHealthkit",
      goap_selectors::closest_selector(goap_selectors::is_healthkit),
      GoapBlackboardKey::CLOSEST_HEALTHKIT_POS));

  actions.emplace_back(std::make_shared<PickupHealthkit>());

  actions.emplace_back(
      std::make_shared<DwellAtOwnBaseWaitingForFlagToBeCaptured>());

  return actions;
}

GoToEntityGoapAction::GoToEntityGoapAction(std::string name,
                                           GoapEntitySelectorFunc target_entity,
                                           GoapBlackboardKey target_pos_key)
    : _name(std::move(name)), _target_entity(target_entity),
      _target_pos_key(target_pos_key) {}

std::string GoToEntityGoapAction::get_name() const { return _name; }

float GoToEntityGoapAction::get_reward(SmartAIThinkCtx &ctx,
                                       const GoapBlackboard &initial_state,
                                       GoapBlackboard &finish_state_out) const {
                                      
  EntityRef target_entity = _target_entity(ctx);
  if (!target_entity.valid(ctx.game)) {
    return -INFINITY;
  }

  Vec2 own_pos = std::get<Vec2>(initial_state.at(GoapBlackboardKey::OWN_POS));

  if (!own_pos.isfinite()) {
    return -INFINITY;
  }

  Vec2 target_pos = target_entity.get(ctx.game).position;

  if (_target_pos_key != GoapBlackboardKey::INVALID &&
      initial_state.contains(_target_pos_key)) {
    target_pos = std::get<Vec2>(initial_state.at(_target_pos_key));
  }

  finish_state_out[GoapBlackboardKey::OWN_POS] = target_pos;

  return own_pos.distance(target_pos) * -0.3f - 10.0f; // MAGICNUMBER
}

void GoToEntityGoapAction::on_begin(SmartAIThinkCtx &ctx) {
  EntityRef target_entity = _target_entity(ctx);
  Vec2 target_pos = Vec2(INFINITY, INFINITY);
  if (!target_entity.valid(ctx.game)) {
    LOG_DEBUG() << "[AI] Target entity for GoToEntityGoapAction is invalid."
                << std::endl;

  } else {
    target_pos = target_entity.get(ctx.game).position;
  }

  LOG_DEBUG() << "[AI] Starting action: " << _name
              << ", target position: " << target_pos << std::endl;

  ctx.state.pathfindingTarget = target_pos;
}

bool GoToEntityGoapAction::perform(SmartAIThinkCtx &ctx) {
  if (ctx.state.path.empty()) {
    return true;
  }
  return false;
}

GoToEnemyFlagGoapAction::GoToEnemyFlagGoapAction()
    : GoToEntityGoapAction(
          "GoToEnemyFlag",
          goap_selectors::first_selector(goap_selectors::is_enemy_flag),
          GoapBlackboardKey::ENEMY_FLAG_POS) {}

float GoToEnemyFlagGoapAction::get_reward(
    SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
    GoapBlackboard &finish_state_out) const {

  if (std::get<bool>(
          initial_state.at(GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG))) {
    return -INFINITY; // Cannot go to enemy flag if we are already holding it
  }

  return GoToEntityGoapAction::get_reward(ctx, initial_state, finish_state_out);
}

PickUpEnemyFlag::PickUpEnemyFlag() {}

std::string PickUpEnemyFlag::get_name() const { return "PickUpEnemyFlag"; }

float PickUpEnemyFlag::get_reward(SmartAIThinkCtx &ctx,
                                  const GoapBlackboard &initial_state,
                                  GoapBlackboard &finish_state_out) const {
  if (std::get<bool>(
          initial_state.at(GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG))) {
    return -INFINITY; // Cannot pickup enemy flag if it is already held by
                      // someone
  }

  Vec2 enemy_flag_pos =
      std::get<Vec2>(initial_state.at(GoapBlackboardKey::ENEMY_FLAG_POS));
  Vec2 own_pos = std::get<Vec2>(initial_state.at(GoapBlackboardKey::OWN_POS));

  if (!enemy_flag_pos.isfinite() || !own_pos.isfinite()) {

    return -INFINITY; // Invalid positions
  }
  float dist = enemy_flag_pos.distance(own_pos);
  if (dist > ctx.game.get_gvar<float>(GVarType::ENTITY_INTERACTION_RANGE)) {

    return -INFINITY; // Too far to pick up the flag
  }

  finish_state_out[GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG] = true;

  return -3.0f; // MAGICNUMBER
}

bool PickUpEnemyFlag::perform(SmartAIThinkCtx &ctx) {

  capnp::MallocMessageBuilder message;
  auto interact = message.initRoot<net::InteractNetMessage>();
  interact.setType(net::InteractionType::USE_INTERACTION);
  interact.setInteractorNetId(ctx.character.net_id);
  ctx.game.perform_interaction(interact);
  return true;
}

PickupHealthkit::PickupHealthkit() {}

std::string PickupHealthkit::get_name() const { return "PickupHealthkit"; }

float PickupHealthkit::get_reward(SmartAIThinkCtx &ctx,
                                  const GoapBlackboard &initial_state,
                                  GoapBlackboard &finish_state_out) const {

  float dist = blackboard_pos_distance(initial_state,
                                       GoapBlackboardKey::CLOSEST_HEALTHKIT_POS,
                                       GoapBlackboardKey::OWN_POS);
  if (dist > ctx.game.get_gvar<float>(GVarType::ENTITY_INTERACTION_RANGE)) {

    return -INFINITY; // Too far to pick up the flag
  }

  finish_state_out[GoapBlackboardKey::HEALTH_PERCENTAGE] = std::clamp(
      std::get<float>(initial_state.at(GoapBlackboardKey::HEALTH_PERCENTAGE)) +
          0.3f, // TODO: use the actual health amount from the healthkit
      0.0f, 1.0f);

  return -3.0f; // MAGICNUMBER
}

bool PickupHealthkit::perform(SmartAIThinkCtx &ctx) {

  capnp::MallocMessageBuilder message;
  auto interact = message.initRoot<net::InteractNetMessage>();
  interact.setType(net::InteractionType::USE_INTERACTION);
  interact.setInteractorNetId(ctx.character.net_id);
  ctx.game.perform_interaction(interact);
  return true;
}

DwellAtOwnBaseWaitingForFlagToBeCaptured::
    DwellAtOwnBaseWaitingForFlagToBeCaptured() {}

std::string DwellAtOwnBaseWaitingForFlagToBeCaptured::get_name() const {
  return "DwellAtOwnBaseWaitingForFlagToBeCaptured";
}

float DwellAtOwnBaseWaitingForFlagToBeCaptured::get_reward(
    SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
    GoapBlackboard &finish_state_out) const {

  float dist_to_own_base = giewont::blackboard_pos_distance(
      initial_state, GoapBlackboardKey::OWN_BASE_POS,
      GoapBlackboardKey::OWN_POS);

  float enemy_flag_dist_to_own_base = giewont::blackboard_pos_distance(
      initial_state, GoapBlackboardKey::ENEMY_FLAG_POS,
      GoapBlackboardKey::OWN_BASE_POS);

  bool is_flag_in_capture_animation = std::get<bool>(
      initial_state.at(GoapBlackboardKey::IS_ENEMY_FLAG_IN_CAPTURED_ANIMATION));
  bool is_holding_enemy_flag = std::get<bool>(
      initial_state.at(GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG));

  float base_activation_dist =
      ctx.game.get_gvar<float>(GVarType::BASE_ACTIVATION_DIST);

  if (is_holding_enemy_flag && dist_to_own_base < base_activation_dist) {
    // We are holding the enemy flag and it is activating our own base.

    finish_state_out[GoapBlackboardKey::IS_ENEMY_FLAG_IN_CAPTURED_ANIMATION] =
        true; // It will be captured soon

    return -1.0f;
  }

  if (is_flag_in_capture_animation && dist_to_own_base < base_activation_dist) {
    // The flag is in the captured animation state and we are close to our base
    // A point will be awarded soon
    return -3.0f;
  }

  return INFINITY;
}

bool DwellAtOwnBaseWaitingForFlagToBeCaptured::perform(SmartAIThinkCtx &ctx) {
  float base_activation_dist =
      ctx.game.get_gvar<float>(GVarType::BASE_ACTIVATION_DIST);
  bool is_flag_in_capture_animation = std::get<bool>(ctx.state.current_state.at(
      GoapBlackboardKey::IS_ENEMY_FLAG_IN_CAPTURED_ANIMATION));
  if (giewont::blackboard_pos_distance(
          ctx.state.current_state, GoapBlackboardKey::ENEMY_FLAG_POS,
          GoapBlackboardKey::OWN_BASE_POS) < base_activation_dist ||
      is_flag_in_capture_animation) {
    return false; // Flag is still being captured, do not finish the action
  }
  return true; // Finished waiting at the base
}
