#include "SmartAICharacterController.h"
#include "CharacterEntity.h"
#include "Entity.h"
#include "FlagEntity.h"
#include "GameplayManager.h"
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

void SmartAICharacterController::update(Game &game, CharacterEntity &character,
                                        float delta_time) {
  SmartAIThinkCtx ctx{game, character, state};
  if (ctx.state.dwellTime > 0.0f) {
    ctx.state.dwellTime -= delta_time;
    return;
  }
  if (ctx.state.path.empty()) {

    EntityRef flag_ref = get_enemy_flag(ctx);
    if (flag_ref.valid_as<FlagEntity>(game)) {
      auto &flag_ent = flag_ref.get_as<FlagEntity>(game);
      Vec2 enemy_flag_pos = flag_ent.position;

      if (flag_ent.flag_holder.valid(game) &&
          flag_ent.flag_holder == character.get_ref()) {
        // We are holding it
        EntityRef base_ref = get_own_base(ctx);
        if (base_ref.valid_as<TeamBaseEntity>(game)) {
          auto &base_ent = base_ref.get_as<TeamBaseEntity>(game);
          Vec2 own_base_pos = base_ent.position;

          this->find_path(ctx, own_base_pos);
        }
      } else {
        if ((character.position - enemy_flag_pos).length() <
            FlagEntity::FLAG_GRAB_DISTANCE) {
          capnp::MallocMessageBuilder message;
          auto interact = message.initRoot<net::InteractNetMessage>();

          interact.setType(net::InteractionType::USE_INTERACTION);
          interact.setInteractorNetId(character.net_id);
          game.perform_interaction(interact);
          ctx.state.dwellTime = rand_float(0.8f, 1.2f);
        } else {
          this->find_path(ctx, enemy_flag_pos);
          ctx.state.dwellTime = rand_float(0.05f, 1.2f);
          ctx.state.waypointReachTime = 0.0f;
        }
      }
    }
  }
  CharacterMovementCommand command = giewont::CharacterMovementCommand::NONE;
  if (!ctx.state.path.empty()) {

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

    // LOG_DEBUG() <<  "feetPos: " << feetPos.x << ", " << feetPos.y
    //             << " currentNode: " << currentNode.world_pos.x << ", "
    //             << currentNode.world_pos.y << std::endl;

    IVec2 feetTilePos =
        ctx.state.last_tilemap->world_pos_to_tilemap_pos(feetPos);
    ctx.state.tilemapFeetPos = feetTilePos;
    if (currentNode.flags & AiPathNodeFlag::LANDING_SITE &&
        !hasReachedCurrentNode) {

      if (feetTilePos == currentNode.tile_pos) {
        hasReachedCurrentNode = true;
      }
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
      auto x_dist = std::abs(currentNode.world_pos.x - feetPos.x);

      if (x_dist > X_CLOSE_THRESHOLD) {
        // Move towards the current node
        bool should_move_right = feetPos.x < currentNode.world_pos.x;

        if (should_move_right) {

          command |= giewont::CharacterMovementCommand::MOVE_RIGHT;
        } else {

          command |= giewont::CharacterMovementCommand::MOVE_LEFT;
        }
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
                        (2.0f * ctx.game.gravity.y);
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
