#include "GameplayManager.h"
#include "CharacterEntity.h"
#include "Color.h"
#include "DataBinder.h"
#include "Entity.h"
#include "FlagEntity.h"
#include "Log.h"
#include "TeamBaseEntity.h"
#include "schema.capnp.h"

#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif

using namespace giewont;

GW_DATABINDER_DEFINE(GameplayManager, GW_DATABINDER_FIELD(int, blue_team_score),
                     GW_DATABINDER_FIELD(int, red_team_score),
                     GW_DATABINDER_FIELD(float, message_time),
                     GW_DATABINDER_FIELD(std::string, message), );

GameplayManager::GameplayManager() : Entity() {
  team_states[GameplayTeam::RED_TEAM] = GameplayTeamState();
  team_states[GameplayTeam::BLUE_TEAM] = GameplayTeamState();

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

void GameplayManager::draw_raylib_ui(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS

  auto blue_team_score_text = "Blue " + std::to_string(blue_team_score);
  auto red_team_score_text = "Red " + std::to_string(red_team_score);

  int right_text_x =
      GetScreenWidth() - MeasureText(blue_team_score_text.c_str(), 40) - 30;

  DrawText(blue_team_score_text.c_str(), right_text_x, 60, 40, BLUE);
  DrawText(red_team_score_text.c_str(), 30, 60, 40, RED);

  auto message_text_x =
      GetScreenWidth() / 2 - MeasureText(message.c_str(), 40) / 2;

  Color message_color = BLACK;

  if (message_time < 1.0f) {
    message_color.a = (unsigned char)(message_time * 255);
  }

  DrawText(message.c_str(), message_text_x, 240, 40, message_color);

#endif
}

void GameplayManager::server_update(Game &game, float delta_time) {
  message_time -= delta_time;
  if (message_time <= 0.0f) {
    message = "";
    message_time = 0.0f;
  }
  time_until_gameplay_state_check -= delta_time;
  if (time_until_gameplay_state_check <= 0.0f) {
    check_gameplay_state(game);
    time_until_gameplay_state_check = 0.5f;
  }
  blue_team_score = team_states[GameplayTeam::BLUE_TEAM].score;
  red_team_score = team_states[GameplayTeam::RED_TEAM].score;
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
            state.score = 0;
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

      if (state.did_create_flag_first_time) {
        // Assign score to the other team
        for (auto &[other_team, other_state] : team_states) {
          if (other_team != team) {
            other_state.score++;
            LOG_INFO() << "Score for team "
                       << gameplay_team_to_string(other_team) << " is "
                       << other_state.score << std::endl;
            if (other_team == GameplayTeam::RED_TEAM) {

              message = "Red team captured the flag!";
              message_time = 3.0f;
            } else {

              message = "Blue team captured the flag!";
              message_time = 3.0f;
            }

            GColor scoring_color = gameplay_team_to_color(other_team);

            capnp::MallocMessageBuilder message_builder;
            auto sync_message = message_builder.initRoot<net::BaseNetMessage>();
            auto add_camera_effect = sync_message.initAddCameraEffect();
            add_camera_effect.setType(net::CameraEffectType::VIGNETTE);
            add_camera_effect.setDuration(1.5f);
            add_camera_effect.setIntensity(0.5f);
            add_camera_effect.setFalloffDuration(0.4f);
            add_camera_effect.setColor(scoring_color.to_uint32());

            game.broadcast_reliable(message_builder);
          }
        }
      }
      state.did_create_flag_first_time = true;
    }
  }
}

void GameplayManager::update_from_sync_message(
    Game &game, const net::SyncEntityNetMessage::Reader &sync_message) {
  Entity::update_from_sync_message(game, sync_message);
  auto gameplay_data = sync_message.getExtraData().getGameplayManagerData();
  blue_team_score = (int)gameplay_data.getBlueTeamScore();
  red_team_score = (int)gameplay_data.getRedTeamScore();

  message = gameplay_data.getMessage().cStr();
  message_time = gameplay_data.getMessageTime();
}

void GameplayManager::build_sync_message(
    Game &game, net::SyncEntityNetMessage::Builder &sync_message) {
  Entity::build_sync_message(game, sync_message);
  auto gameplay_data = sync_message.getExtraData().initGameplayManagerData();
  gameplay_data.setBlueTeamScore((uint32_t)blue_team_score);
  gameplay_data.setRedTeamScore((uint32_t)red_team_score);

  gameplay_data.setMessage(message.c_str());
  gameplay_data.setMessageTime(message_time);
}

void GameplayManager::spawn_player_with_team(Game &game, uint32_t peer_id,
                                             GameplayTeam team) {
  EntityRef player_ref = game.spawn_player_character(
      peer_id, team_states[team].base.get(game).position);

  auto &player = player_ref.get_as<CharacterEntity>(game);

  player.team = team;

  LOG_INFO() << "spawning player with team " << gameplay_team_to_string(player.team)
             << std::endl;


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
