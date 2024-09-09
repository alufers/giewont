#include "GameplayManager.h"
#include "Color.h"
#include "FlagEntity.h"
#include "Log.h"
#include "TeamBaseEntity.h"

using namespace giewont;

GW_DATABINDER_DEFINE(GameplayManager, );

GameplayManager::GameplayManager() : Entity() {
  team_states[GameplayTeam::RED_TEAM] = GaemplayTeamState();
  team_states[GameplayTeam::BLUE_TEAM] = GaemplayTeamState();

  LOG_INFO() << "GameplayManager created, n_teams" << team_states.size()
             << std::endl;
}

void GameplayManager::load_assets(const Game &game) {}

void GameplayManager::update(Game &game, float delta_time) {
  if (game.is_server()) {
    server_update(game, delta_time);
  }
}

void GameplayManager::draw(const Game &game) {}

void GameplayManager::server_update(Game &game, float delta_time) {
  time_until_gameplay_state_check -= delta_time;
  if (time_until_gameplay_state_check <= 0.0f) {
    check_gameplay_state(game);
    time_until_gameplay_state_check = 3.0f;
  }
}

void GameplayManager::check_gameplay_state(Game &game) {

  for (auto &[team, state] : team_states) {
    if (!state.base.valid(game)) {
      // We need to find a base
      for (auto &entity : game.valid_entities()) {
        if (TeamBaseEntity *base =
                dynamic_cast<TeamBaseEntity *>(entity.get())) {
          if (base->team == GameplayTeam::UNKNOWN_TEAM) {
            base->team = team;
            base->color = gameplay_team_to_color(team);
            state.base = entity->get_ref();
            break;
          }
        }
      }
      if (!state.base.valid(game)) {
        LOG_ERROR() << "No base found for team "
                    << gameplay_team_to_string(team) << std::endl;
        return;
      }
    }
  }

  // Checks if each team has a flag
  for (auto &[team, state] : team_states) {
    if (!state.flag.valid(game)) {
      auto flag = std::make_unique<FlagEntity>();
      flag->position =
          state.base.get_as<TeamBaseEntity>(game).get_flag_spawn_pos();
      flag->team = team;
      flag->color = gameplay_team_to_color(team);
      flag->load_assets(game);

      LOG_INFO() << "Flag created for team " << gameplay_team_to_string(team)
                 << std::endl;
      state.flag = game.push_entity(std::move(flag));
    }
  }
}

std::string giewont::gameplay_team_to_string(GameplayTeam team) {
  switch (team) {
  case GameplayTeam::RED_TEAM:
    return "RED_TEAM";
  case GameplayTeam::BLUE_TEAM:
    return "BLUE_TEAM";
  default:
    return "UNKNOWN_TEAM";
  }
}

GColor giewont::gameplay_team_to_color(GameplayTeam team) {
  switch (team) {
  case GameplayTeam::RED_TEAM:
    return GColor(1.0f, 0.0f, 0.0f, 1.0f);
  case GameplayTeam::BLUE_TEAM:
    return GColor(0.0f, 0.0f, 1.0f, 1.0f);
  default:
    return GColor(1.0f, 1.0f, 1.0f, 1.0f);
  }
}
