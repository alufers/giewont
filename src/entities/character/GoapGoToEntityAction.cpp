
#include "GoapGoToEntityAction.h"
#include "Log.h"
#include "SmartAIContainers.h"
#include "math/RandUtil.h"

using namespace giewont;

const float WAYPOINT_REACHED_THRESHOLD = 38.0f;
const float X_CLOSE_THRESHOLD = 20.0f;
const float WAYPOINT_REACH_MAX_TIME = 5.0f;

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

  if (std::get<Vec2>(initial_state.at(GoapBlackboardKey::OWN_POS))
          .distance(target_pos) < WAYPOINT_REACHED_THRESHOLD) {
    return -INFINITY; // Don't do anything if we are already close enough
  }

  finish_state_out[GoapBlackboardKey::OWN_POS] = target_pos;

  return own_pos.distance(target_pos) * -0.3f - 10.0f; // MAGICNUMBER
}

GoapActionResult GoToEntityGoapAction::on_begin(SmartAIThinkCtx &ctx) {
  ctx.state.goToEntityTarget = _target_entity(ctx);

  if (!ctx.state.goToEntityTarget.valid(ctx.game)) {
    LOG_DEBUG() << "[AI] Target entity for GoToEntityGoapAction is invalid."
                << std::endl;
    return GoapActionResult::FAILED_FORCE_REPLAN; // Retrying won't help, the
                                                  // entity is gone
  }

  Vec2 target_pos = ctx.state.goToEntityTarget.get(ctx.game).position;
  LOG_DEBUG() << "[AI] Starting action: " << _name
              << ", target position: " << target_pos << std::endl;

  this->find_path(ctx, target_pos);
  ctx.state.cancelScheduled = false;

  if (ctx.state.path.empty()) {
    LOG_DEBUG() << "[AI] No path found for GoToEntityGoapAction." << std::endl;
    return GoapActionResult::FAILED_RECOVERABLE; // Retrying might help
  }

  LOG_DEBUG() << "[AI] Path found for GoToEntityGoapAction, size: "
              << ctx.state.path.size() << std::endl;
  return GoapActionResult::IN_PROGRESS;
}

GoapActionResult GoToEntityGoapAction::perform(SmartAIThinkCtx &ctx) {

  GoapActionResult result = GoapActionResult::IN_PROGRESS;
  CharacterMovementCommand command = giewont::CharacterMovementCommand::NONE;

  if (ctx.state.path.empty()) {
    LOG_DEBUG() << "[AI] Path buffer empty, GoToEntityGoapAction completed."
                << std::endl;
    result = GoapActionResult::DONE;
  } else {

    // Check whether the target entity still exists and is close enough to the
    // end
    // of the path.
    if (!ctx.state.goToEntityTarget.valid(ctx.game)) {
      LOG_DEBUG()
          << "[AI] Target entity for GoToEntityGoapAction became invalid "
             "while following path."
          << std::endl;
      // Do not abort the movement immediately as it may cause the character to
      // fall off a ladder. Delay it until the next waypoint is reached.
      ctx.state.cancelScheduled = true;
    } else if (ctx.state.path.back().world_pos.distance(
                   ctx.state.goToEntityTarget.get(ctx.game).position) >
               ctx.game.get_gvar<float>(GVarType::ENTITY_INTERACTION_RANGE)) {
      LOG_DEBUG()
          << "[AI] Target entity for GoToEntityGoapAction is too far away, "
             "replanning."
          << std::endl;
      ctx.state.cancelScheduled = true;
    }

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

    // If a cancel was scheduled, then we should abort now
    if (ctx.state.cancelScheduled && hasReachedCurrentNode) {
      result = GoapActionResult::FAILED_FORCE_REPLAN;
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

    ctx.state.waypointReachTime += ctx.delta_time;
    if (ctx.state.waypointReachTime > WAYPOINT_REACH_MAX_TIME) {
      ctx.state.path.clear();
      ctx.state.dwellTime = rand_float(0.05f, 1.2f);
      if (rand_float(0.0f, 1.0f) < 0.5f) {
        command |= giewont::CharacterMovementCommand::MOVE_LEFT;
      }
      result = GoapActionResult::FAILED_RECOVERABLE;
    }
  }
  ctx.character.perform_movement(ctx.game, ctx.delta_time, command);
  return result;
}

void GoToEntityGoapAction::find_path(SmartAIThinkCtx &ctx, Vec2 target_pos) {

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

void GoToEntityGoapAction::reconstruct_path(SmartAIThinkCtx &ctx,
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
