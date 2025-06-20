#include "SmartAICharacterController.h"
#include "CharacterEntity.h"
#include "Entity.h"
#include "FlagEntity.h"
#include "GameplayManager.h"
#include "GoapActions.h"
#include "GoapGoals.h"
#include "IVec2.h"
#include "Log.h"
#include "TeamBaseEntity.h"
#include "TilemapEntity.h"
#include "math/RandUtil.h"
#include <cmath>
#include <cstdlib>
#include <raylib.h>
#include <vector>

using namespace giewont;

SmartAICharacterController::SmartAICharacterController()
    : CharacterController() {
  state.sensors = construct_goap_sensors();
  state.goals = construct_goap_goals();
  state.actions = construct_goap_actions();
}

void SmartAICharacterController::update(Game &game, CharacterEntity &character,
                                        float delta_time) {
  SmartAIThinkCtx ctx{game, character, state, delta_time};

  state.dwellTime -= delta_time;
  state.timeUntilPlanReevaluation -= delta_time;
  state.currentPlanAge += delta_time;

  if (state.dwellTime > 0.0f) {
    return;
  }

  // Goap stuff
  this->process_sensors(ctx);
  this->process_goals(ctx);
  this->process_actions(ctx);
  this->generate_goap_plan(ctx);

  if (!ctx.state.currentGoapPlan.empty()) {
    auto &currentPlainItem = ctx.state.currentGoapPlan.front();
  }

  bool did_do_any_goap_action = false;
  for (auto &plan_item : ctx.state.currentGoapPlan) {
    if (plan_item.did_complete)
      continue;
    if (std::shared_ptr<GoapAction> action = plan_item.action.lock()) {
      GoapActionResult result;

      did_do_any_goap_action = true;
      if (!plan_item.did_begin) {
        LOG_DEBUG() << "[AI] Starting action: " << action->get_name()
                    << std::endl;
        result = action->on_begin(ctx);
      } else {

        result = action->perform(ctx);
      }
      plan_item.last_result = result;
      switch (result) {
      case GoapActionResult::NOT_ATTEMPTED:
        throw std::runtime_error(
            "[AI] Action " + action->get_name() +
            " returned NOT_ATTEMPTED, this should not happen.");
        break;
      case GoapActionResult::FAILED_FORCE_REPLAN:
        LOG_DEBUG() << "[AI] Action " << action->get_name()
                    << " failed, replanning." << std::endl;
        state.dwellTime = rand_float(0.5f, 3.2f);
        return; // No point in continuing, we need to replan
      case GoapActionResult::FAILED_RECOVERABLE:
        LOG_DEBUG() << "[AI] Action " << action->get_name()
                    << " failed, but can be retried." << std::endl;
        plan_item.did_begin = false;
        plan_item.attempt_count++;
        state.dwellTime = rand_float(0.1f, 1.2f);
        if (plan_item.attempt_count > 5) {
          LOG_DEBUG() << "[AI] Action " << action->get_name()
                      << " failed too many times, replanning." << std::endl;
          plan_item.last_result =
              GoapActionResult::FAILED_FORCE_REPLAN; // Force replan
        }
        // We can retry the action, so we don't clear the plan
        return;
      case GoapActionResult::IN_PROGRESS:
        plan_item.did_begin = true;
        break;
      case GoapActionResult::DONE:
        LOG_DEBUG() << "[AI] Action " << action->get_name()
                    << " completed successfully." << std::endl;
        plan_item.did_begin = true;
        plan_item.did_complete = true;
        break;
      case GoapActionResult::RESTART_IMMEDIATE:
        plan_item.did_begin = false; // Restart the action immediately
        return; // No point in continuing, we need to replan
      }

      break; // We only execute one action at a time
    }
  }

  if (!did_do_any_goap_action) {
    LOG_DEBUG() << "[AI] GOAP plan completed" << std::endl;
    ctx.state.dwellTime = rand_float(0.5f, 3.2f);
  }

  if (state.timeUntilPlanReevaluation <= 0.0f) {
    state.timeUntilPlanReevaluation = state.planReevaluationInterval;
    LOG_DEBUG() << "[AI] Re-evaluating GOAP plan" << std::endl;
    this->evaluate_current_goap_plan(ctx);
  }
}

void SmartAICharacterController::draw_debug(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  // Draw the path if it exists

  if (state.last_tilemap != nullptr && !state.aStarData.empty()) {
    for (int x = 0; x < state.last_tilemap->tilemap_width; x++) {
      for (int y = 0; y < state.last_tilemap->tilemap_height; y++) {
        auto node = state.aStarData[x + y * state.last_tilemap->tilemap_width];
        if (node != nullptr) {

          DrawCircle(node->world_pos.x, node->world_pos.y,
                     node->isGoal ? 15.0f : 2.0f, BLUE);
        }
      }
    }
  }

  if (!state.path.empty()) {
    for (size_t i = 0; i < state.path.size() - 1; ++i) {
      DrawLineEx(state.path[i].world_pos.to_raylib(),
                 state.path[i + 1].world_pos.to_raylib(), 2.0f, RED);
      DrawCircle(state.path[i].world_pos.x, state.path[i].world_pos.y, 3.0f,
                 RED);
      if (state.path[i].flags & AiPathNodeFlag::LANDING_SITE) {
        // draw a cross
        DrawLineEx((state.path[i].world_pos - Vec2(5, 5)).to_raylib(),
                   (state.path[i].world_pos + Vec2(5, 5)).to_raylib(), 2.0f,
                   GREEN);
        DrawLineEx((state.path[i].world_pos + Vec2(5, -5)).to_raylib(),
                   (state.path[i].world_pos - Vec2(5, -5)).to_raylib(), 2.0f,
                   GREEN);
      }

      if (state.path[i].flags & AiPathNodeFlag::JUMP_BEFORE_REACHING) {
        // draw a cross
        DrawLineEx((state.path[i].world_pos - Vec2(5, 0)).to_raylib(),
                   (state.path[i].world_pos + Vec2(5, 0)).to_raylib(), 2.0f,
                   BLACK);
        DrawLineEx((state.path[i].world_pos + Vec2(0, -5)).to_raylib(),
                   (state.path[i].world_pos - Vec2(0, -5)).to_raylib(), 2.0f,
                   BLACK);
      }
    }
  }

  if (state.last_tilemap != 0) {
    Vec2 tileWorldPos =
        state.last_tilemap->position +
        Vec2(state.tilemapFeetPos.x * state.last_tilemap->tile_size.x,
             state.tilemapFeetPos.y * state.last_tilemap->tile_size.y);
    DrawRectangle(tileWorldPos.x, tileWorldPos.y,
                  state.last_tilemap->tile_size.x,
                  state.last_tilemap->tile_size.y, BLUE);
  }

#endif
};

void SmartAICharacterController::process_sensors(SmartAIThinkCtx &ctx) {
  for (auto &sensor : ctx.state.sensors) {

    if (sensor->last_update_frame % sensor->sense_frequency == 0) {
      sensor->last_update_frame = 0;
      GoapBlackboardValue value = sensor->sense(ctx);
      ctx.state.current_state[sensor->get_key()] = value;
    }
    sensor->last_update_frame++;
  }
}

void SmartAICharacterController::process_goals(SmartAIThinkCtx &ctx) {
  for (auto &goal : ctx.state.goals) {
    goal->_last_reward_value =
        goal->get_reward(ctx, ctx.state.current_state, ctx.state.current_state);
  }
}

void SmartAICharacterController::process_actions(SmartAIThinkCtx &ctx) {
  for (auto &action : ctx.state.actions) {
    GoapBlackboard finishState = ctx.state.current_state;
    float cost = action->get_reward(ctx, ctx.state.current_state, finishState);
    action->_last_reward_value = cost;
  }
}

void SmartAICharacterController::generate_goap_plan(SmartAIThinkCtx &ctx) {

  bool has_any_performable_actions = false;
  for (const auto &plan_step : ctx.state.currentGoapPlan) {
    GoapActionResult res = plan_step.last_result;
    if (res == GoapActionResult::FAILED_FORCE_REPLAN) {
      has_any_performable_actions = false;
      break; // One action failed unrecoverably, we need to replan
    }

    if (res != GoapActionResult::DONE) {
      has_any_performable_actions = true;
    }
  }

  if (has_any_performable_actions)
    return; // The current plan is still valid.

  std::vector<GoapPlanItem> initial_plan;
  auto next_plan = this->consider_next_plan_item(ctx, initial_plan);
  if (next_plan.has_value()) {
    LOG_DEBUG() << "[AI] Found a plan with " << next_plan.value().size()
                << " items" << std::endl;

    for (const auto &item : next_plan.value()) {
      LOG_DEBUG() << "[AI] Action: " << item.action.lock()->get_name()
                  << ", Action Reward: " << item.action_reward
                  << ", Goal Reward: " << item.goal_reward << std::endl;
    }
    this->state.currentGoapPlan = next_plan.value();

    this->state.currentPlanAge = 0.0f;
    if (!this->state.currentGoapPlan.empty()) {
      this->state.currentPlanExpectedGoalReward =
          this->state.currentGoapPlan.back().goal_reward;
    } else {
      this->state.currentPlanExpectedGoalReward = 0.0f;
    }

  } else {
    LOG_DEBUG() << "[AI] No plan found" << std::endl;
  }
}

std::optional<std::vector<GoapPlanItem>>
SmartAICharacterController::consider_next_plan_item(
    SmartAIThinkCtx &ctx, std::vector<GoapPlanItem> const &curr_plan) {
  if (curr_plan.size() > 4) {
    return std::nullopt; // Too long plan, don't consider it
  }

  GoapBlackboard initial_state = ctx.state.current_state;
  if (!curr_plan.empty()) {
    initial_state = curr_plan.back().state;
  }

  std::optional<std::vector<GoapPlanItem>> best_plan = std::nullopt;
  float best_plan_reward = -INFINITY;
  if (!curr_plan.empty()) {
    // If we already have a plan, use it's reward as the best one,
    // to prevent taking nonsense paths
    best_plan_reward = curr_plan.back().goal_reward;
    for (const auto &item : curr_plan) {
      best_plan_reward += item.action_reward;
    }
  }

  for (auto &action : ctx.state.actions) {
    GoapBlackboard finish_state = initial_state;
    float cost = action->get_reward(ctx, initial_state, finish_state);
    if (!std::isnan(cost) && std::isfinite(cost)) {

      apply_gameplay_logic_to_predicted_blackboard(ctx, finish_state);

      float goal_reward = 0.0f;
      for (auto &goal : ctx.state.goals) {
        goal_reward += goal->get_reward(ctx, initial_state, finish_state);
      }

      // This action can be performed
      std::vector<GoapPlanItem> new_plan = curr_plan;
      new_plan.push_back(GoapPlanItem{
          .action = action,
          .state = finish_state,
          .action_reward = cost,
          .goal_reward = goal_reward,
      });

      // Now recusively consider the next plan item after the action
      auto next_plan = this->consider_next_plan_item(ctx, new_plan);
      if (next_plan.has_value()) {
        new_plan = next_plan.value();
      }

      // Calculate the total reward of the plan
      float total_reward = 0.0f;
      for (const auto &item : new_plan) {
        total_reward += item.action_reward; // Sum up action rewards
      }
      total_reward +=
          new_plan[new_plan.size() - 1]
              .goal_reward; // Add the goal reward for the last state

      if (total_reward > best_plan_reward) {

        best_plan_reward = total_reward;
        best_plan = new_plan; // Update the best plan if this one is better
      }
    }
  }

  return best_plan;
}

void SmartAICharacterController::evaluate_current_goap_plan(
    SmartAIThinkCtx &ctx) {
  if (ctx.state.currentGoapPlan.empty()) {
    return; // Nothing to evaluate
  }

  bool needs_replanning = false;
  GoapBlackboard bb_state = ctx.state.current_state;
  for (auto &plan_item : ctx.state.currentGoapPlan) {
    if (plan_item.last_result == GoapActionResult::DONE)
      continue; // Skip completed actions
    if (plan_item.last_result == GoapActionResult::FAILED_FORCE_REPLAN)
      break; // Plan is already invalid don't check further
    if (plan_item.action.expired()) {
      needs_replanning = true;
      break; // Action is no longer valid, we need to replan
    }

    GoapBlackboard finish_state = bb_state;
    float reward =
        plan_item.action.lock()->get_reward(ctx, bb_state, finish_state);
    if (!std::isfinite(reward) || std::isnan(reward)) {
      LOG_DEBUG() << "[AI] Action " << plan_item.action.lock()->get_name()
                  << " returned invalid reward, replanning." << std::endl;
      needs_replanning = true;
      break; // Invalid reward, we need to replan
    }
    apply_gameplay_logic_to_predicted_blackboard(ctx, finish_state);
    float goal_reward = 0.0f;
    for (auto &goal : ctx.state.goals) {
      goal_reward += goal->get_reward(ctx, bb_state, finish_state);
    }
    plan_item.action_reward = reward;
    plan_item.state = finish_state;
    plan_item.goal_reward = goal_reward;
    bb_state = finish_state; // Update the state for the next action
  }

  if (needs_replanning) {
    LOG_DEBUG() << "[AI] =======PLAN NEEDS REPLANNING AFTER EVALUATION====="
                << std::endl;
    ctx.state.currentGoapPlan.clear();
  }
}

void SmartAICharacterController::draw_inspector_ui(Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS

  ImGui::Separator();
  ImGui::Text("Current plan:");
  ImGui::Text("Plan expected goal reward: %.2f",
              state.currentPlanExpectedGoalReward);
  ImGui::Text("Plan age: %.2f", state.currentPlanAge);
  if (ImGui::Button("Force replan")) {
    state.currentGoapPlan.clear();
  }
  ImGui::BeginTable("Action State", 4, ImGuiTableFlags_Borders);
  ImGui::TableSetupColumn("Action");
  ImGui::TableSetupColumn("Result");
  ImGui::TableSetupColumn("Action Reward");
  ImGui::TableSetupColumn("Goal Reward");
  ImGui::TableHeadersRow();
  for (const auto &plan_item : state.currentGoapPlan) {
    ImGui::TableNextRow();
    if (plan_item.last_result == GoapActionResult::IN_PROGRESS) {
      ImGui::TableSetBgColor(
          ImGuiTableBgTarget_RowBg0,
          ImGui::GetColorU32(ImVec4(0.0f, 0.3f, 0.0f, 0.3f)));
    } else if (plan_item.last_result == GoapActionResult::DONE) {
      ImGui::TableSetBgColor(
          ImGuiTableBgTarget_RowBg0,
          ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 0.3f)));
    } else if (plan_item.last_result == GoapActionResult::FAILED_RECOVERABLE) {
      ImGui::TableSetBgColor(
          ImGuiTableBgTarget_RowBg0,
          ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.3f, 0.3f)));
    }
    ImGui::TableNextColumn();
    ImGui::Text("%s", plan_item.action.lock()->get_name().c_str());
    ImGui::TableNextColumn();
    ImGui::Text("%s",
                goap_action_result_to_string(plan_item.last_result).c_str());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f", plan_item.action_reward);
    ImGui::TableNextColumn();
    ImGui::Text("%.2f", plan_item.goal_reward);
  }
  ImGui::EndTable();

  ImGui::Separator();
  ImGui::Text("Current sensor state:");
  ImGui::BeginTable("Sensor State", 2, ImGuiTableFlags_Borders);

  for (const auto &pair : state.current_state) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("%s", goap_blackboard_key_to_string(pair.first).c_str());
    ImGui::TableNextColumn();
    ImGui::Text("%s", goap_blackboard_value_to_string(pair.second).c_str());
  }
  ImGui::EndTable();

  ImGui::Separator();
  ImGui::Text("Current goal state:");
  ImGui::BeginTable("Goal State", 2, ImGuiTableFlags_Borders);
  for (const auto &goal : state.goals) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("%s", goal->get_name().c_str());
    ImGui::TableNextColumn();
    ImGui::Text("Reward: %.2f", goal->_last_reward_value);
  }
  ImGui::EndTable();

  ImGui::Separator();
  ImGui::Text("Current action state:");
  ImGui::BeginTable("action State", 2, ImGuiTableFlags_Borders);
  for (const auto &action : state.actions) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("%s", action->get_name().c_str());
    ImGui::TableNextColumn();
    ImGui::Text("Reward: %.2f", action->_last_reward_value);
  }
  ImGui::EndTable();

#endif
}
