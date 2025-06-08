#pragma once

#include "CharacterEntity.h"
#include "Game.h"
#include "IVec2.h"
#include "TilemapEntity.h"
#include "GoapInterfaces.h"
#include "GoapSensors.h"

namespace giewont {

enum class AiPathNodeFlag : uint32_t {
  NONE = 0,

  /// @brief A landing site has a more relaxed destinationCheck, as we expect
  /// some overshoot, which will most likely move us towards the next node
  LANDING_SITE = 1 << 0,

  LADDER_CLIMB = 1 << 1,

  JUMP_BEFORE_REACHING = 1 << 2,
};

constexpr AiPathNodeFlag operator|(AiPathNodeFlag a, AiPathNodeFlag b) {
  return static_cast<AiPathNodeFlag>(static_cast<uint32_t>(a) |
                                     static_cast<uint32_t>(b));
}

constexpr bool operator&(AiPathNodeFlag a, AiPathNodeFlag b) {
  return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
}

struct AiPathNode {
  Vec2 world_pos;
  IVec2 tile_pos;

  float gScore = 0.0;
  float fScore = 0.0;
  bool isGoal = false;
  AiPathNode *parent = nullptr;

  AiPathNodeFlag flags = AiPathNodeFlag::NONE;
};
// State persisted across updates
struct SmartAIState {

  // Goap stuff
  GoapBlackboard current_state;
  std::vector<std::unique_ptr<GoapSensor>> sensors;
  std::vector<std::unique_ptr<GoapGoal>> goals;
  std::vector<std::unique_ptr<GoapAction>> actions;

  // Pathfinding stuff
  std::vector<AiPathNode> path;

  TilemapEntity *last_tilemap = nullptr;
  // Contains pathfinding information about nodes calculated in the last search
  // Owns the nodes
  std::vector<AiPathNode *> aStarData;
  IVec2 tilemapFeetPos = IVec2(0, 0);
  bool did_jump_for_current_waypoint = false;
  float dwellTime = 1.0f;
  float waypointReachTime = 0.0f;
  int noPathAttempts = 0.0f;
  float enemyAttachCheckTime = 10.0f;
};

struct SmartAIThinkCtx {
  Game &game;
  CharacterEntity &character;
  SmartAIState &state;
};

} // namespace giewont
