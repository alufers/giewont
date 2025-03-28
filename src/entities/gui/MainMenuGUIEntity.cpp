#include "MainMenuGUIEntity.h"
#include "Log.h"

#if GIEWONT_HAS_GRAPHICS
#include "ClientGame.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#endif

#include <iostream>
#include <random>
#include <vector>

using namespace giewont;

static std::string generateRandomUsername() {
  // List of adjectives
  std::vector<std::string> adjectives = {"Swift", "Clever", "Brave",  "Happy",
                                         "Eager", "Jolly",  "Lively", "Witty",
                                         "Zesty", "Merry"};

  // List of verbs
  std::vector<std::string> verbs = {"Jump", "Run",   "Fly",   "Swim", "Sing",
                                    "Dash", "Laugh", "Climb", "Spin", "Dance"};

  // Random number generator setup
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> adjDist(0, adjectives.size() - 1);
  std::uniform_int_distribution<int> verbDist(0, verbs.size() - 1);
  std::uniform_int_distribution<int> numDist(10, 99); // Two-digit number

  // Generate random username
  std::string username = adjectives[adjDist(gen)] + verbs[verbDist(gen)] +
                         std::to_string(numDist(gen));
  return username;
}

void MainMenuGUIEntity::load_assets(const Game &game) {
  player_name_text = generateRandomUsername();
}

void MainMenuGUIEntity::update(Game &game, float delta_time) {}

void MainMenuGUIEntity::draw(const Game &game) {}

void MainMenuGUIEntity::draw_imgui_ui(Game &game) {
#if GIEWONT_HAS_GRAPHICS
  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetNextWindowPos(
      ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
      ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::Begin("Main Menu");
  ImGui::InputText("Player Name", &player_name_text);
  ImGui::InputText("Server Address", &server_addr_text);
  if (ImGui::Button("Connect")) {
    LOG_INFO() << "Connect button pressed" << std::endl;
    ClientGame &client_game = dynamic_cast<ClientGame &>(game);

    // Try parsing port

    std::string host = server_addr_text;
    int port = 1338;
    size_t colon_pos = server_addr_text.rfind(':');
    if (colon_pos != std::string::npos) {
      std::string port_str = server_addr_text.substr(colon_pos + 1);
      if (!port_str.empty() &&
          std::all_of(port_str.begin(), port_str.end(), ::isdigit)) {
        port = std::stoi(port_str);
        host = server_addr_text.substr(0, colon_pos);
      }
    }

    client_game.connect_to_server(server_addr_text, 1338, player_name_text);
  }

  if (ImGui::Button("Quit Game")) {
    LOG_INFO() << "Quit button pressed" << std::endl;
  }
  ImGui::End();
#endif
}
