#pragma once

#include "Entity.h"
#include <string>

namespace giewont {
class MainMenuGUIEntity : public Entity {
public:
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;
  void draw_imgui_ui(Game &game) override;

private:
  std::string server_addr_text = "localhost";
  std::string player_name_text = "Player";
};
}; // namespace giewont
