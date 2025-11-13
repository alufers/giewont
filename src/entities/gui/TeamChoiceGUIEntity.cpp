#include "TeamChoiceGUIEntity.h"
#include "Game.h"
#include "Log.h"
#if GIEWONT_HAS_GRAPHICS
#include "ClientGame.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#endif

#include "GameplayManager.h"
#include "schema.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace giewont;

void TeamChoiceGUIEntity::load_assets(const Game &game) {}

void TeamChoiceGUIEntity::update(Game &game, float delta_time) {}

void TeamChoiceGUIEntity::draw(const Game &game) {}

void TeamChoiceGUIEntity::draw_imgui_ui(Game &game) {
  if (game.my_peer_id != this->net_owner_peer_id) {
    return; // Not our dialog
  }
#if GIEWONT_HAS_GRAPHICS
  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetNextWindowPos(
      ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
      ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::Begin("Choose a team");

  if (ImGui::Button("BLUE TEAM")) {
    capnp::MallocMessageBuilder message_builder;
    auto sync_message = message_builder.initRoot<net::BaseNetMessage>();
    auto gui_interaction = sync_message.initGuiInteraction();
    gui_interaction.setNetId(this->net_id);
    gui_interaction.setName("BLUE_TEAM_CHOSEN");

    game.send_reliable_to_peer(0, message_builder);

    // Store for analytics
    static_cast<ClientGame &>(game).own_team_name = "BLUE_TEAM";
  }

  if (ImGui::Button("RED TEAM")) {
    capnp::MallocMessageBuilder message_builder;
    auto sync_message = message_builder.initRoot<net::BaseNetMessage>();
    auto gui_interaction = sync_message.initGuiInteraction();
    gui_interaction.setNetId(this->net_id);
    gui_interaction.setName("RED_TEAM_CHOSEN");

    game.send_reliable_to_peer(0, message_builder);
    static_cast<ClientGame &>(game).own_team_name = "BLUE_TEAM";

  }
  ImGui::End();
#endif
}

void TeamChoiceGUIEntity::handle_gui_interaction(
    Game &game, const net::GuiInteractionNetMessage::Reader interaction) {

  GameplayTeam team = GameplayTeam::UNKNOWN_TEAM;
  if (interaction.getName() == "BLUE_TEAM_CHOSEN") {
    LOG_INFO() << "Blue team chosen" << std::endl;
    team = GameplayTeam::BLUE_TEAM;
  }
  if (interaction.getName() == "RED_TEAM_CHOSEN") {
    LOG_INFO() << "Red team chosen" << std::endl;
    team = GameplayTeam::RED_TEAM;
  }

  if (team == GameplayTeam::UNKNOWN_TEAM) {
    LOG_WARN() << "Unknown team chosen" << std::endl;
    return;
  }
  for (auto &entity : game.entities) {
    if (!entity || entity->id == this->id || entity->marked_for_deletion) {
      continue;
    }
    if (GameplayManager *mgr = dynamic_cast<GameplayManager *>(entity.get())) {
      mgr->spawn_player_with_team(game, this->net_owner_peer_id, team);
      this->destroy();
      return;
      ;
    }
  }

  LOG_ERROR() << "No GameplayManager found" << std::endl;
}
