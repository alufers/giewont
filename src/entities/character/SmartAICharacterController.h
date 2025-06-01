#pragma once

#include "CharacterController.h"
#include "Game.h"
#include "IVec2.h"
#include "SmartAIContainers.h"
#include "Vec2.h"

#include "TilemapEntity.h"
#include <optional>
#include <vector>

namespace giewont {

// SmartAICharacterController controls a character using pathfinding.
class SmartAICharacterController : public CharacterController {
public:
  SmartAICharacterController();
  void update(Game &game, CharacterEntity &character,
              float delta_time) override;

  void draw_debug(const Game &game) override;

  void draw_inspector_ui(Game &game) override;

private:
  void process_sensors(SmartAIThinkCtx &ctx);

  /// @brief Find a path to the target position using A* algorithm
  void find_path(SmartAIThinkCtx &ctx, Vec2 target_pos);

  /// @brief Fill in state.path with the path from start to goal given a goal
  /// node
  void reconstruct_path(SmartAIThinkCtx &ctx, AiPathNode *goalNode);

  EntityRef get_own_base(SmartAIThinkCtx &ctx);
  EntityRef get_enemy_flag(SmartAIThinkCtx &ctx);
  SmartAIState state;
};

} // namespace giewont
