#include "GoapActions.h"
#include "Log.h"
#include "RandUtil.h"
#include "SmartAIContainers.h"

using namespace giewont;

std::vector<std::shared_ptr<GoapAction>> giewont::construct_goap_actions() {
  std::vector<std::shared_ptr<GoapAction>> actions;
  actions.emplace_back(std::make_shared<GoToEnemyFlagGoapAction>());
  actions.emplace_back(std::make_shared<FollowEnemyFlagAction>());
  actions.emplace_back(std::make_shared<PickUpEnemyFlag>());
  actions.emplace_back(std::make_shared<DropEnemyFlag>());

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
  actions.emplace_back(std::make_shared<JumpAndMoveLeft>());

  return actions;
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

FollowEnemyFlagAction::FollowEnemyFlagAction()
    : GoToEntityGoapAction(
          "FollowToEnemyFlag",
          goap_selectors::first_selector(goap_selectors::is_enemy_flag),
          GoapBlackboardKey::ENEMY_FLAG_POS) {
  allow_target_entity_movement = true; // Allow the enemy flag to move while we
                                       // are following it
}

float FollowEnemyFlagAction::get_reward(
    SmartAIThinkCtx &ctx, const GoapBlackboard &initial_state,
    GoapBlackboard &finish_state_out) const {

  if (std::get<bool>(
          initial_state.at(GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG))) {
    return -INFINITY; // Cannot go to enemy flag if we are already holding it
  }

  if (!std::get<bool>(
          initial_state.at(GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG))) {
    return -INFINITY; // Aonly allow following the flag if somebody is holding
                      // it
  }

  // Make following the flag more rewarding than going to it once.
  return GoToEntityGoapAction::get_reward(ctx, initial_state,
                                          finish_state_out) +
         10.0f;
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

GoapActionResult PickUpEnemyFlag::perform(SmartAIThinkCtx &ctx) {
  if (ctx.character.position.distance(std::get<Vec2>(
          ctx.state.current_state.at(GoapBlackboardKey::ENEMY_FLAG_POS))) >
      ctx.game.get_gvar<float>(GVarType::ENTITY_INTERACTION_RANGE)) {

    return GoapActionResult::FAILED_FORCE_REPLAN;
  }
  capnp::MallocMessageBuilder message;
  auto interact = message.initRoot<net::InteractNetMessage>();
  interact.setType(net::InteractionType::USE_INTERACTION);
  interact.setInteractorNetId(ctx.character.net_id);
  ctx.game.perform_interaction(interact);
  return GoapActionResult::DONE;
}

DropEnemyFlag::DropEnemyFlag() {}

std::string DropEnemyFlag::get_name() const { return "DropEnemyFlag"; }

float DropEnemyFlag::get_reward(SmartAIThinkCtx &ctx,
                                const GoapBlackboard &initial_state,
                                GoapBlackboard &finish_state_out) const {
  if (!std::get<bool>(
          initial_state.at(GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG))) {
    return -INFINITY; // Cannot pickup enemy flag if it is already held by
                      // someone
  }

  finish_state_out[GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG] = false;
  finish_state_out[GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG] = false;

  return -60.0f; // MAGICNUMBER
}

GoapActionResult DropEnemyFlag::perform(SmartAIThinkCtx &ctx) {
  
  capnp::MallocMessageBuilder message;
  auto interact = message.initRoot<net::InteractNetMessage>();
  interact.setType(net::InteractionType::USE_INTERACTION);
  interact.setInteractorNetId(ctx.character.net_id);
  ctx.game.perform_interaction(interact);
  ctx.state.dwellTime = rand_float(0.5f, 3.2f); // Wait a bit for the flag to move away
                                            
  return GoapActionResult::DONE;
}

PickupHealthkit::PickupHealthkit() {}

std::string PickupHealthkit::get_name() const { return "PickupHealthkit"; }

float PickupHealthkit::get_reward(SmartAIThinkCtx &ctx,
                                  const GoapBlackboard &initial_state,
                                  GoapBlackboard &finish_state_out) const {

  if (std::get<bool>(
          initial_state.at(GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG))) {
    return -INFINITY; // Cannot pickup healthkit if we are holding the enemy
                      // flag
  }
  float dist = blackboard_pos_distance(initial_state,
                                       GoapBlackboardKey::CLOSEST_HEALTHKIT_POS,
                                       GoapBlackboardKey::OWN_POS);
  if (dist > ctx.game.get_gvar<float>(GVarType::ENTITY_INTERACTION_RANGE)) {

    return -INFINITY; // Too far to pick up the healthkit
  }

  finish_state_out[GoapBlackboardKey::HEALTH_PERCENTAGE] = std::clamp(
      std::get<float>(initial_state.at(GoapBlackboardKey::HEALTH_PERCENTAGE)) +
          0.3f, // TODO: use the actual health amount from the healthkit
      0.0f, 1.0f);

  finish_state_out[GoapBlackboardKey::CLOSEST_HEALTHKIT_POS] =
      Vec2(INFINITY, INFINITY);

  return -3.0f; // MAGICNUMBER
}

GoapActionResult PickupHealthkit::perform(SmartAIThinkCtx &ctx) {
  if (ctx.character.position.distance(std::get<Vec2>(ctx.state.current_state.at(
          GoapBlackboardKey::CLOSEST_HEALTHKIT_POS))) >
      ctx.game.get_gvar<float>(GVarType::ENTITY_INTERACTION_RANGE)) {

    return GoapActionResult::FAILED_FORCE_REPLAN;
  }
  capnp::MallocMessageBuilder message;
  auto interact = message.initRoot<net::InteractNetMessage>();
  interact.setType(net::InteractionType::USE_INTERACTION);
  interact.setInteractorNetId(ctx.character.net_id);
  ctx.game.perform_interaction(interact);
  return GoapActionResult::DONE;
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

  return -INFINITY;
}

GoapActionResult
DwellAtOwnBaseWaitingForFlagToBeCaptured::perform(SmartAIThinkCtx &ctx) {
  float base_activation_dist =
      ctx.game.get_gvar<float>(GVarType::BASE_ACTIVATION_DIST);
  bool is_flag_in_capture_animation = std::get<bool>(ctx.state.current_state.at(
      GoapBlackboardKey::IS_ENEMY_FLAG_IN_CAPTURED_ANIMATION));
  if (giewont::blackboard_pos_distance(
          ctx.state.current_state, GoapBlackboardKey::ENEMY_FLAG_POS,
          GoapBlackboardKey::OWN_BASE_POS) < base_activation_dist ||
      is_flag_in_capture_animation) {
    return GoapActionResult::IN_PROGRESS; // Flag is still being captured, do
                                          // not finish the action
  }
  return GoapActionResult::DONE; // Finished waiting at the base
}

JumpAndMoveLeft::JumpAndMoveLeft() {}

std::string JumpAndMoveLeft::get_name() const { return "JumpAndMoveLeft"; }

float JumpAndMoveLeft::get_reward(SmartAIThinkCtx &ctx,
                                  const GoapBlackboard &initial_state,
                                  GoapBlackboard &finish_state_out) const {

  bool is_propped =
      std::get<bool>(initial_state.at(GoapBlackboardKey::IS_PROPPED));
  bool is_in_water =
      std::get<bool>(initial_state.at(GoapBlackboardKey::IS_IN_WATER));

  if (!is_propped && !is_in_water) {
    return -INFINITY; // Cannot jump if not propped or in water
  }
  finish_state_out[GoapBlackboardKey::IS_IN_WATER] = false;

  return -10.0f;
}

GoapActionResult JumpAndMoveLeft::perform(SmartAIThinkCtx &ctx) {
  CharacterMovementCommand cmd =
      CharacterMovementCommand::MOVE_LEFT | CharacterMovementCommand::JUMP;
  ctx.character.perform_movement(ctx.game, ctx.delta_time, cmd);
  ctx.state.dwellTime = rand_float(0.05f, 0.40f);
  return GoapActionResult::DONE; // Finished waiting at the base
}
