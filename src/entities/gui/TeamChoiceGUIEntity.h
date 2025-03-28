#pragma once

#include "Entity.h"

namespace giewont {
class TeamChoiceGUIEntity : public Entity {
public:
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;
  void draw_imgui_ui(Game &game) override;

  void handle_gui_interaction(
      Game &game,
      const net::GuiInteractionNetMessage::Reader interaction) override;

  net::EntityType get_net_type() override {
    return net::EntityType::TEAM_CHOICE_G_U_I;
  }
  const char *get_type_name() const override { return "TeamChoiceGUIEntity"; }

private:
};
}; // namespace giewont
