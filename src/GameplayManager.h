#ifndef GAMEPLAYMANAGER_H_
#define GAMEPLAYMANAGER_H_

#include "Color.h"
#include "DataBinder.h"
#include "Entity.h"
#include <unordered_map>

namespace giewont {

enum class GameplayTeam { UNKNOWN_TEAM = 0, RED_TEAM = 1, BLUE_TEAM = 2 };

std::string gameplay_team_to_string(GameplayTeam team);
GColor gameplay_team_to_color(GameplayTeam team);
GameplayTeam gameplay_team_get_enemy(GameplayTeam team);

class GameplayTeamState {
public:
  bool did_create_flag_first_time = false;
  bool did_spawn_ai_first_time = false;
  int score = 0;
  EntityRef base;
  EntityRef flag;
};

class GameplayManager : public Entity {
public:
  GameplayManager();

  const char *get_type_name() const override { return "GameplayManager"; }
  net::EntityType get_net_type() override {
    return net::EntityType::GAMEPLAY_MANAGER;
  }
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;
  void draw_raylib_ui(const Game &game) override;

  // synced state
  int blue_team_score = 0;
  int red_team_score = 0;
  std::string message = "";
  float message_time = 0.0f;

  void update_from_sync_message(
      Game &game,
      const net::SyncEntityNetMessage::Reader &sync_message) override;

  void
  build_sync_message(Game &game,
                     net::SyncEntityNetMessage::Builder &sync_message) override;

  GW_DATABINDER_DECLARE(GameplayManager);
  GW_DATABINDER_AUTO_INSPECTOR(klass);

  float time_until_gameplay_state_check = 0.5f;

  std::unordered_map<GameplayTeam, GameplayTeamState> team_states;

  // Serverside methods
  void spawn_player_with_team(Game &game, uint32_t peer_id, GameplayTeam team);
  void notify_player_died(Game &game, EntityRef player);

private:
  // Config
  size_t initial_ai_spawn_count = 5; // Per team

  void server_update(Game &game, float delta_time);

  /**
   * @brief Called every 3 seconds to check the state of the game, and validate
   * any gameplay entities.
   *
   * @param game
   */
  void check_gameplay_state(Game &game);
};

} // namespace giewont

#endif // GAMEPLAYMANAGER_H_
