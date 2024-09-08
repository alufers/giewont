#ifndef DEBUGGUI_H_
#define DEBUGGUI_H_

#include "Entity.h"
#include "Game.h"
#include <vector>

namespace giewont {
class DebugGUI {
public:
  bool show = false;
  bool show_entity_inspector = false;
  bool show_debug_overlay = false;

  // Inspector
  std::string inspector_filter;
  EntityRef inspector_selected_entity;

  void draw(Game &game, std::vector<EntityRef> &entity_draw_list);
  void draw_ui(Game &game);
  void update(Game &game);
};
}; // namespace giewont

#endif // DEBUGGUI_H_
