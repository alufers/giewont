#pragma once

#include "CharacterEntity.h"
#include "Game.h"
#include "IVec2.h"
#include "TilemapEntity.h"
#include "GoapInterfaces.h"
#include "GoapSensors.h"
#include <atomic>

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

constexpr AiPathNodeFlag operator~(AiPathNodeFlag a) {
  return static_cast<AiPathNodeFlag>(~static_cast<uint32_t>(a));
}

constexpr AiPathNodeFlag operator&=(AiPathNodeFlag &a, AiPathNodeFlag b) {
  a = static_cast<AiPathNodeFlag>(static_cast<uint32_t>(a) &
                                  static_cast<uint32_t>(b));
  return a;
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
  std::vector<std::shared_ptr<GoapAction>> actions;
  std::vector<GoapPlanItem> currentGoapPlan;
  std::atomic<bool> is_generating_plan = false;
  std::vector<GoapPlanItem> plan_from_threadpool;

  float currentPlanAge = 0.0f;
  std::map<std::string, float> expectedGoalRewards;
  float planReevaluationInterval = 2.0f;
  float timeUntilPlanReevaluation = 2.0f;



  // State used by the GoToEntity action
  EntityRef goToEntityTarget;
  std::vector<AiPathNode> path;
  bool cancelScheduled = false;
  TilemapEntity *last_tilemap = nullptr;
  size_t tilemap_not_found_fail_count = 0; // For destrying self if we can't find a tilemap
  size_t waypoint_reach_time_fail_count = 0;
  bool needs_nudge = false; // Set to false after failing to follow a waypoint

  // Stuff used by the GoToEnemy action
  size_t num_frames_with_line_of_sight_to_enemy = 0;

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
  float delta_time = 0.0f;
};

} // namespace giewont
