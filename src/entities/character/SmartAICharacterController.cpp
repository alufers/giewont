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

const float WAYPOINT_REACHED_THRESHOLD = 38.0f;
const float X_CLOSE_THRESHOLD = 20.0f;
const float WAYPOINT_REACH_MAX_TIME = 5.0f;

SmartAICharacterController::SmartAICharacterController()
    : CharacterController() {
  state.sensors = construct_goap_sensors();
  state.goals = construct_goap_goals();
  state.actions = construct_goap_actions();
}

void SmartAICharacterController::update(Game &game, CharacterEntity &character,
                                        float delta_time) {
  SmartAIThinkCtx ctx{game, character, state};

  // Goap stuff
  this->process_sensors(ctx);
  this->process_goals(ctx);
  this->process_actions(ctx);
  this->generate_goap_plan(ctx);

  if (!ctx.state.currentGoapPlan.empty()) {
    auto &currentPlainItem = ctx.state.currentGoapPlan.front();
    if (std::shared_ptr<GoapAction> action = currentPlainItem.action.lock()) {
      if (currentPlainItem.did_begin == false) {
        LOG_DEBUG() << "[AI] Starting action: " << action->get_name()
                    << std::endl;

        GoapBlackboard uselessState;
        float reward =
            action->get_reward(ctx, ctx.state.current_state, uselessState);
        if (std::isnan(reward) || !std::isfinite(reward)) {
          LOG_DEBUG() << "[AI] Action " << action->get_name()
                      << " can no longer be performed, replanning."
                      << std::endl;
          ctx.state.currentGoapPlan.clear();

        } else {
          action->on_begin(ctx);
          currentPlainItem.did_begin = true;
        }
      } else {
        bool done = action->perform(ctx);
        if (done) {
          LOG_DEBUG() << "[AI] Finished action: " << action->get_name()
                      << std::endl;
          ctx.state.currentGoapPlan.erase(ctx.state.currentGoapPlan.begin());
        }
      }
    }
  }

  if (ctx.state.path.empty() && ctx.state.pathfindingTarget.isfinite()) {
    ctx.state.noPathAttempts++;

    if (ctx.state.noPathAttempts > 20) {
      character.health = 0;
    }

    this->find_path(ctx, ctx.state.pathfindingTarget);
    LOG_DEBUG() << "[AI] Pathfinding to target: " << ctx.state.pathfindingTarget
                << " nodes: " << ctx.state.path.size() << std::endl;
  }
  CharacterMovementCommand command = giewont::CharacterMovementCommand::NONE;
  if (!ctx.state.path.empty()) {
    ctx.state.noPathAttempts = 0;
    // LOG_DEBUG() << "Path size: " << ctx.state.path.size() << std::endl;
    auto feetPos = ctx.character.world_feet_pos() - Vec2(0, 1.0f);
    auto &currentNode = ctx.state.path[0];

    if ((currentNode.flags & AiPathNodeFlag::JUMP_BEFORE_REACHING) &&
        !ctx.state.did_jump_for_current_waypoint &&
        ctx.character.is_propped_by_level) {
      command |= giewont::CharacterMovementCommand::JUMP;
      ctx.state.did_jump_for_current_waypoint = true;
    }

    bool hasReachedCurrentNode =
        currentNode.world_pos.distance(feetPos) < WAYPOINT_REACHED_THRESHOLD;

    IVec2 feetTilePos =
        ctx.state.last_tilemap->world_pos_to_tilemap_pos(feetPos);

    float min_target_x = currentNode.world_pos.x;
    float max_target_x = currentNode.world_pos.x;

    // Permit overshooting if the next node iso on the same y
    if (ctx.state.path.size() > 1) {
      auto nextNode = ctx.state.path[1];
      if (nextNode.tile_pos.y == currentNode.tile_pos.y &&
          !(nextNode.flags & AiPathNodeFlag::JUMP_BEFORE_REACHING)) {
        min_target_x = std::min(currentNode.world_pos.x, nextNode.world_pos.x);
        max_target_x = std::max(currentNode.world_pos.x, nextNode.world_pos.x);

        if (std::abs(feetPos.y - nextNode.world_pos.y) <
                WAYPOINT_REACHED_THRESHOLD / 2.0 &&
            (feetPos.x > min_target_x) && (feetPos.x < max_target_x)) {
          LOG_DEBUG() << "Reached node by overshooting to the next one"
                      << std::endl;
          hasReachedCurrentNode = true;
        }
      }
    }

    ctx.state.tilemapFeetPos = feetTilePos;

    if (currentNode.flags & AiPathNodeFlag::LANDING_SITE &&
        !hasReachedCurrentNode) {
      if (feetTilePos == currentNode.tile_pos) {
        hasReachedCurrentNode = true;
      }
    }

    // wait for stopping when reaching the target
    if (hasReachedCurrentNode && ctx.state.path.size() == 1) {
      hasReachedCurrentNode = ctx.character.velocity.length() < 30.0f;
    }

    if (hasReachedCurrentNode) {
      ctx.state.did_jump_for_current_waypoint = false;
      ctx.state.waypointReachTime = 0.0f;
      // We are close to the node, remove it from the path
      ctx.state.path.erase(ctx.state.path.begin());
      if (ctx.state.path.empty()) {
        ctx.state.dwellTime = rand_float(0.05f, 3.2f);
      }
    } else {

      // auto x_dist = std::abs(currentNode.world_pos.x - feetPos.x);

      // if (x_dist > X_CLOSE_THRESHOLD) {
      //   // Move towards the current node
      //   bool should_move_right = feetPos.x < currentNode.world_pos.x;

      //   if (should_move_right) {

      //     command |= giewont::CharacterMovementCommand::MOVE_RIGHT;
      //   } else {

      //     command |= giewont::CharacterMovementCommand::MOVE_LEFT;
      //   }
      // }

      if (min_target_x - feetPos.x > X_CLOSE_THRESHOLD) {
        command |= giewont::CharacterMovementCommand::MOVE_RIGHT;
      } else if (feetPos.x - max_target_x > X_CLOSE_THRESHOLD) {
        command |= giewont::CharacterMovementCommand::MOVE_LEFT;
      }

      bool should_jump = (feetPos.y - currentNode.world_pos.y) > 10.0f;
      if (should_jump) {
        command |= giewont::CharacterMovementCommand::JUMP;
      }
    }

    ctx.state.waypointReachTime += delta_time;
    if (ctx.state.waypointReachTime > WAYPOINT_REACH_MAX_TIME) {
      ctx.state.path.clear();
      ctx.state.dwellTime = rand_float(0.05f, 1.2f);
      if (rand_float(0.0f, 1.0f) < 0.5f) {
        command |= giewont::CharacterMovementCommand::MOVE_LEFT;
      }
    }
  }
  character.perform_movement(game, delta_time, command);
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

EntityRef SmartAICharacterController::get_own_base(SmartAIThinkCtx &ctx) {
  for (auto &entity : ctx.game.valid_entities()) {
    if (TeamBaseEntity *base = dynamic_cast<TeamBaseEntity *>(entity.get())) {
      if (base->team == ctx.character.team) {
        return entity->get_ref();
      }
    }
  }
  return EntityRef(); // Return an invalid EntityRef if no base is found
}

EntityRef SmartAICharacterController::get_enemy_flag(SmartAIThinkCtx &ctx) {
  for (auto &entity : ctx.game.valid_entities()) {
    if (FlagEntity *flag = dynamic_cast<FlagEntity *>(entity.get())) {
      if (flag->team == gameplay_team_get_enemy(ctx.character.team)) {
        return entity->get_ref();
      }
    }
  }
  return EntityRef(); // Return an invalid EntityRef if no base is found
}

void SmartAICharacterController::find_path(SmartAIThinkCtx &ctx,
                                           Vec2 target_pos) {

  Vec2 feet_pos = ctx.character.world_feet_pos() - Vec2(0, 5.0f);
  target_pos -= Vec2(0, 5.0f);
  TilemapEntity *tilemap = nullptr;
  for (auto &entity : ctx.game.valid_entities()) {
    if (TilemapEntity *tm = dynamic_cast<TilemapEntity *>(entity.get())) {
      if (tm->is_point_in_tilemap_bounds(target_pos) &&
          tm->is_point_in_tilemap_bounds(feet_pos)) {
        tilemap = tm;
      }
      break;
    }
  }
  if (tilemap == nullptr) {
    LOG_ERROR() << "No tilemap found for pathfinding" << std::endl;
    return;
  }

  ctx.state.last_tilemap = tilemap;

  auto nearestWalkableTargetPosOption =
      tilemap->get_nearest_walkable_tile_pos(target_pos);
  if (!nearestWalkableTargetPosOption.has_value()) {
    LOG_ERROR() << "No walkable tile found for pathfinding" << std::endl;
    return;
  }

  auto nearestWalkableStartPosOption =
      tilemap->get_nearest_walkable_tile_pos(feet_pos);

  if (!nearestWalkableStartPosOption.has_value()) {
    LOG_ERROR() << "No walkable tile found for pathfinding" << std::endl;
    return;
  }

  // Calculate character capabilities
  float maxJumpHeight = (ctx.character.jump_speed * ctx.character.jump_speed) /
                        (2.0f * ctx.game.get_gvar<Vec2>(GVarType::GRAVITY).y);
  float characterHeight = ctx.character.get_aabb().height();
  int charcterHeightInTiles =
      static_cast<int>(std::ceil(characterHeight / tilemap->tile_size.y));

  for (auto node : ctx.state.aStarData) {
    // cleanup last search (we keep it in-between updates for debugging)
    if (node != nullptr) {
      delete (node);
    }
  }

  ctx.state.aStarData.clear();

  auto startTile = nearestWalkableStartPosOption.value();
  auto targetTile = nearestWalkableTargetPosOption.value();

  LOG_DEBUG() << "[AI] Start tile: " << startTile << std::endl;
  LOG_DEBUG() << "[AI] Target tile: " << targetTile << std::endl;

  auto &nodes = ctx.state.aStarData;
  nodes.resize(tilemap->tilemap_width *
               tilemap->tilemap_height); // 1D array of nodes

  auto heuristic = [targetTile](IVec2 from) -> float {
    return (from - targetTile).length2();
  };

  AiPathNode *startNode = new AiPathNode{
      .world_pos = tilemap->get_tile_bottom_center_world_pos(startTile),
      .tile_pos = startTile,
      .gScore = 0.0f,
      .fScore = heuristic(targetTile),
      .isGoal = false};

  nodes[startTile.x + startTile.y * tilemap->tilemap_width] = startNode;

  nodes[targetTile.x + targetTile.y * tilemap->tilemap_width] = new AiPathNode{
      .world_pos = tilemap->get_tile_bottom_center_world_pos(targetTile),
      .tile_pos = targetTile,
      .gScore = INFINITY,
      .fScore = 0.0f,
      .isGoal = true,
  };

  std::vector<AiPathNode *> openSet;
  openSet.push_back(startNode);

  auto processNeighbour = [&](AiPathNode *comingFrom, IVec2 neighbourPos,
                              float cost,
                              AiPathNodeFlag flags = AiPathNodeFlag::NONE) {
    auto &neighborNode =
        nodes[neighbourPos.x + neighbourPos.y * tilemap->tilemap_width];
    if (neighborNode == nullptr) {
      neighborNode = new AiPathNode{
          .world_pos = tilemap->get_tile_bottom_center_world_pos(neighbourPos),
          .tile_pos = neighbourPos,
          .gScore = INFINITY,
          .fScore = INFINITY,
          .isGoal = false};
    }

    float tentativeGScore = comingFrom->gScore + cost;
    if (tentativeGScore < neighborNode->gScore) {
      neighborNode->parent = comingFrom;
      neighborNode->gScore = tentativeGScore;
      neighborNode->fScore = tentativeGScore + heuristic(neighbourPos);
      neighborNode->flags = flags;
      if (std::find(openSet.begin(), openSet.end(), neighborNode) ==
          openSet.end()) {
        openSet.push_back(neighborNode);
      }
    }
  };

  auto canHaveFeetInTile = [&](IVec2 tilePos) {
    for (int i = 0; i < charcterHeightInTiles; i++) {
      auto tileType = tilemap->get_tile_type_at(tilePos - IVec2(0, i));
      if (tileType != TileType::AIR && tileType != TileType::LADDER) {
        return false;
      }
    }
    return true;
  };

  /// @brief Given a tile position check where we will end up if we enter it
  /// from the side
  auto generateNeighboursAtColumn = [&](IVec2 columnPos,
                                        AiPathNode *comingFrom) {
    // First try looking what would happen if we fell down (or just walked
    // over the tile)
    auto currTile = columnPos;
    while (tilemap->is_tile_in_bounds(currTile)) {
      bool can_stand_in_tile = canHaveFeetInTile(currTile);

      if (!can_stand_in_tile) {
        break;
      }
      TileType tileBelowFeet =
          tilemap->get_tile_type_at(currTile + IVec2(0, 1));
      bool can_stand_on_tile_below =
          tileBelowFeet == TileType::SOLID || tileBelowFeet == TileType::LADDER;

      if (can_stand_in_tile && can_stand_on_tile_below) {
        float cost = std::abs(currTile.x - comingFrom->tile_pos.x) * 10.0f +
                     std::abs(currTile.y - comingFrom->tile_pos.y) * 20.0f;
        // We can fall (or walk) down here
        processNeighbour(comingFrom, currTile, cost,
                         (currTile.y == comingFrom->tile_pos.y)
                             ? AiPathNodeFlag::NONE
                             : AiPathNodeFlag::LANDING_SITE);
        break;
      }
      currTile.y++;
    }

    TileType comingFromTileType =
        tilemap->get_tile_type_at(comingFrom->tile_pos);
    TileType tileAboveComingFrom =
        tilemap->get_tile_type_at(comingFrom->tile_pos + IVec2(0, 1));
    if (comingFromTileType == TileType::LADDER &&
        tileAboveComingFrom == TileType::LADDER) {
      return; // Do not allow jumping from non-ending ladder tiles
    }

    // Try looking if we can jump up
    currTile = columnPos - IVec2(0, 1);

    while (tilemap->is_tile_in_bounds(currTile) &&
           (std::abs((currTile - comingFrom->tile_pos).y *
                     tilemap->tile_size.y) < maxJumpHeight)) {
      bool can_stand_in_tile = canHaveFeetInTile(currTile);

      TileType tileBelowFeet =
          tilemap->get_tile_type_at(currTile + IVec2(0, 1));
      bool can_stand_on_tile_below =
          tileBelowFeet == TileType::SOLID || tileBelowFeet == TileType::LADDER;

      if (can_stand_in_tile && can_stand_on_tile_below) {
        float cost = std::abs(currTile.x - comingFrom->tile_pos.x) * 10.0f +
                     std::abs(currTile.y - comingFrom->tile_pos.y) * 20.0f;
        if (tileBelowFeet == TileType::LADDER) {
          cost += 220.0f;
        }
        // We can jump up here
        processNeighbour(comingFrom, currTile, cost);
        break;
      }
      currTile.y--;
    }
  };

  auto generateNeighboursForHoleJump = [&](IVec2 targetPos,
                                           AiPathNode *comingFrom) {
    if (!tilemap->is_tile_in_bounds(targetPos) ||
        !canHaveFeetInTile(targetPos)) {
      return;
    }

    // First check for clearance
    int minXToCheck = std::min(comingFrom->tile_pos.x, targetPos.x);
    int maxXToCheck = std::max(comingFrom->tile_pos.x, targetPos.x);
    int minYToCheck = std::min(comingFrom->tile_pos.y - charcterHeightInTiles,
                               targetPos.y - charcterHeightInTiles);
    int maxYToCheck = std::max(comingFrom->tile_pos.y, targetPos.y);

    for (int x = minXToCheck; x <= maxXToCheck; x++) {
      for (int y = minYToCheck; y <= maxYToCheck; y++) {
        auto tileType = tilemap->get_tile_type_at(IVec2(x, y));
        if (tileType != TileType::AIR && tileType != TileType::LADDER) {
          return;
        }
      }
    }

    // Now let's check if we can stand on the target tile
    IVec2 targetFeetTilePos = targetPos + IVec2(0, 1);
    bool canStandOnTargetTile =
        tilemap->get_tile_type_at(targetFeetTilePos) == TileType::SOLID ||
        tilemap->get_tile_type_at(targetFeetTilePos) == TileType::LADDER;
    if (!canStandOnTargetTile) {
      return;
    }

    processNeighbour(
        comingFrom, targetPos,
        std::abs(targetPos.x - comingFrom->tile_pos.x) * 60.0f +
            std::abs(targetPos.y - comingFrom->tile_pos.y) * 20.0f + 210.0f,
        AiPathNodeFlag::LANDING_SITE | AiPathNodeFlag::JUMP_BEFORE_REACHING);
  };

  while (!openSet.empty()) {
    AiPathNode *currentNode = openSet[0];
    for (auto node : openSet) {
      if (node->fScore < currentNode->fScore) {
        currentNode = node;
      }
    }

    // LOG_DEBUG() << "[AI] Current node: " << currentNode->tile_pos
    //             << " isGoal: " << currentNode->isGoal
    //             << " fScore: " << currentNode->fScore << std::endl;
    if (currentNode->isGoal) {
      LOG_INFO() << "[AI] Found path to goal" << std::endl;
      // We found the goal
      reconstruct_path(ctx, currentNode);
      return;
    }

    // Remove current node from openSet
    openSet.erase(std::remove(openSet.begin(), openSet.end(), currentNode),
                  openSet.end());

    // Process neighbors at the sides

    generateNeighboursAtColumn(currentNode->tile_pos + IVec2(1, 0),
                               currentNode);
    generateNeighboursAtColumn(currentNode->tile_pos + IVec2(-1, 0),
                               currentNode);

    generateNeighboursForHoleJump(currentNode->tile_pos + IVec2(-2, 0),
                                  currentNode);
    generateNeighboursForHoleJump(currentNode->tile_pos + IVec2(2, 0),
                                  currentNode);
    generateNeighboursForHoleJump(currentNode->tile_pos + IVec2(-3, 0),
                                  currentNode);
    generateNeighboursForHoleJump(currentNode->tile_pos + IVec2(3, 0),
                                  currentNode);

    TileType tileInFeet = tilemap->get_tile_type_at(currentNode->tile_pos);
    // Check if can climb directly upwards
    if (tileInFeet == TileType::LADDER) {
      processNeighbour(currentNode, currentNode->tile_pos - IVec2(0, 1), 35.0f,
                       AiPathNodeFlag::LADDER_CLIMB);
    }
  }

  LOG_ERROR() << "[AI] No path found" << std::endl;
}

void SmartAICharacterController::reconstruct_path(SmartAIThinkCtx &ctx,
                                                  AiPathNode *goalNode) {

  ctx.state.path.clear();
  AiPathNode *currentNode = goalNode;
  while (currentNode != nullptr) {
    AiPathNode nodeCopy = *currentNode;
    nodeCopy.parent = nullptr; // Avoid copying the parent pointer
    ctx.state.path.push_back(nodeCopy);
    currentNode = currentNode->parent;
  }
  std::reverse(ctx.state.path.begin(), ctx.state.path.end());

  // Now let's clean up the path a bit
  // We mainly want to remove redundant nodes to prevent the AI from jolting
  // about

  for (size_t i = 1; i < ctx.state.path.size() - 1; ++i) {
    // If a node has no flags and has the same y coordinate as the previous and
    // next node we can
    // remove it

    auto &prevNode = ctx.state.path[i - 1];
    auto &currNode = ctx.state.path[i];
    auto &nextNode = ctx.state.path[i + 1];
    if (!(currNode.flags & AiPathNodeFlag::LADDER_CLIMB) &&
        !(currNode.flags & AiPathNodeFlag::JUMP_BEFORE_REACHING) &&
        !(nextNode.flags & AiPathNodeFlag::JUMP_BEFORE_REACHING)) {
      if (prevNode.tile_pos.y == currNode.tile_pos.y &&
          nextNode.tile_pos.y == currNode.tile_pos.y) {
        ctx.state.path.erase(ctx.state.path.begin() + i);
        i--;
      }
    }

    if (currNode.flags & AiPathNodeFlag::LADDER_CLIMB &&
        prevNode.flags & AiPathNodeFlag::LADDER_CLIMB) {
      if (prevNode.tile_pos.x == currNode.tile_pos.x &&
          nextNode.tile_pos.x == currNode.tile_pos.x) {
        ctx.state.path.erase(ctx.state.path.begin() + i);
        i--;
      }
    }
  }
}

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
  // for (auto &action : ctx.state.actions) {
  //   GoapBlackboard finishState = ctx.state.current_state;
  //   float cost = action->get_reward(ctx, ctx.state.current_state,
  //   finishState); action->_last_reward_value = cost;
  // }
}

void SmartAICharacterController::generate_goap_plan(SmartAIThinkCtx &ctx) {
  if (this->state.currentGoapPlan.size() > 0) {
    return;
  }

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

void SmartAICharacterController::draw_inspector_ui(Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
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
  ImGui::Text("Current plan:");
  ImGui::BeginTable("Action State", 3, ImGuiTableFlags_Borders);
  ImGui::TableSetupColumn("Action");
  ImGui::TableSetupColumn("Action Reward");
  ImGui::TableSetupColumn("Goal Reward");
  ImGui::TableHeadersRow();
  for (const auto &plan_item : state.currentGoapPlan) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("%s", plan_item.action.lock()->get_name().c_str());
    ImGui::TableNextColumn();
    ImGui::Text("%.2f", plan_item.action_reward);
    ImGui::TableNextColumn();
    ImGui::Text("%.2f", plan_item.goal_reward);
  }
  ImGui::EndTable();

  ImGui::Separator();
  ImGui::Text("Current goal state:");
  ImGui::BeginTable("Goal State", 2);
  for (const auto &goal : state.goals) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("%s", goal->get_name().c_str());
    ImGui::TableNextColumn();
    ImGui::Text("Reward: %.2f", goal->_last_reward_value);
  }
  ImGui::EndTable();

#endif
}
