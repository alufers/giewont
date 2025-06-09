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
  void process_goals(SmartAIThinkCtx &ctx);
  void process_actions(SmartAIThinkCtx &ctx);
  void generate_goap_plan(SmartAIThinkCtx &ctx);
  std::optional<std::vector<GoapPlanItem>> consider_next_plan_item(SmartAIThinkCtx &ctx, std::vector<GoapPlanItem> const &curr_plan);

 


  SmartAIState state;
};

} // namespace giewont
