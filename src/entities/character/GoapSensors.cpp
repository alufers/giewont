#include "GoapSensors.h"
#include "FlagEntity.h"
#include "GoapInterfaces.h"
#include "Log.h"
#include "SmartAIContainers.h"
#include "TeamBaseEntity.h"
#include "entities/debug/DebugMarkerEntity.h"
using namespace giewont;
using namespace giewont::goap_selectors;

std::vector<std::unique_ptr<GoapSensor>> giewont::construct_goap_sensors() {
  std::vector<std::unique_ptr<GoapSensor>> sensors;

  sensors.push_back(std::make_unique<IsProppedSensor>());
  sensors.push_back(std::make_unique<NeedsNudgeSensor>());
  sensors.push_back(std::make_unique<HealthPercentageSensor>(
      GoapBlackboardKey::HEALTH_PERCENTAGE, first_selector(is_self)));
  sensors.push_back(std::make_unique<IsInWaterSensor>());

  // Distance sensors

  sensors.push_back(std::make_unique<PositionSensor>(GoapBlackboardKey::OWN_POS,
                                                     first_selector(is_self)));
  sensors.push_back(std::make_unique<PositionSensor>(
      GoapBlackboardKey::ENEMY_FLAG_POS, first_selector(is_enemy_flag)));
  sensors.push_back(std::make_unique<PositionSensor>(
      GoapBlackboardKey::ENEMY_BASE_POS, first_selector(is_enemy_base)));
  sensors.push_back(std::make_unique<PositionSensor>(
      GoapBlackboardKey::OWN_FLAG_POS, closest_selector(is_friendly_flag)));
  sensors.push_back(std::make_unique<PositionSensor>(
      GoapBlackboardKey::OWN_BASE_POS, first_selector(is_friendly_base)));
  sensors.push_back(
      std::make_unique<PositionSensor>(GoapBlackboardKey::CLOSEST_ENEMY_POS,
                                       closest_selector(is_enemy_character)));
  sensors.push_back(std::make_unique<PositionSensor>(
      GoapBlackboardKey::CLOSEST_FRIENDLY_POS,
      closest_selector(
          and_filter(is_friendly_character, negate_filter(is_self)))));
  sensors.push_back(
      std::make_unique<PositionSensor>(GoapBlackboardKey::CLOSEST_HEALTHKIT_POS,
                                       closest_selector(is_healthkit)));

  // Enemy character sensors
  sensors.push_back(std::make_unique<HasLineOfSightSensor>(
      GoapBlackboardKey::HAS_LINE_OF_SIGHT_TO_CLOSEST_ENEMY,
      closest_selector(is_enemy_character)));
  sensors.push_back(std::make_unique<HealthPercentageSensor>(
      GoapBlackboardKey::CLOSEST_ENEMY_HEALTH_PERCENTAGE,
      closest_selector(is_enemy_character)));

  // Flag state sensors

  sensors.push_back(std::make_unique<FlagStateSensor>(
      GoapBlackboardKey::IS_ANYBODY_HOLDING_OWN_FLAG,
      first_selector(is_friendly_flag), is_valid_entity));

  sensors.push_back(std::make_unique<FlagStateSensor>(
      GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG,
      first_selector(is_enemy_flag), is_valid_entity));

  sensors.push_back(std::make_unique<FlagStateSensor>(
      GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG, first_selector(is_enemy_flag),
      is_self));
  sensors.push_back(std::make_unique<FlagStateSensor>(
      GoapBlackboardKey::IS_ENEMY_FLAG_IN_CAPTURED_ANIMATION,
      first_selector(is_enemy_flag), is_friendly_base));

  // Add other sensors here as needed
  return sensors;
}

GoapBlackboardValue IsProppedSensor::sense(SmartAIThinkCtx &ctx) {
  bool is_propped = ctx.character.is_propped_by_level;
  return GoapBlackboardValue(is_propped);
}

GoapBlackboardValue NeedsNudgeSensor::sense(SmartAIThinkCtx &ctx) {

  return GoapBlackboardValue( ctx.state.needs_nudge);
}

GoapBlackboardValue IsInWaterSensor::sense(SmartAIThinkCtx &ctx) {
  return GoapBlackboardValue(ctx.character.is_in_water);
}

GoapBlackboardValue HealthPercentageSensor::sense(SmartAIThinkCtx &ctx) {
  EntityRef target_entity = target_selector(ctx);
  if (!target_entity.valid(ctx.game)) {
    return GoapBlackboardValue(NAN); // No target or actor found
  }
  auto &character = target_entity.get_as<CharacterEntity>(ctx.game);

  float health_percentage = static_cast<float>(character.health) /
                            static_cast<float>(character.max_health);
  return GoapBlackboardValue(health_percentage);
}

GoapBlackboardValue PositionSensor::sense(SmartAIThinkCtx &ctx) {
  if (!this->selected_entity.valid(ctx.game) ||
      (entity_scan_counter % 20) == 0) {
    this->selected_entity = target_selector(ctx);
  }

  entity_scan_counter++;
  if (!this->selected_entity.valid(ctx.game)) {
    return GoapBlackboardValue(
        Vec2(INFINITY, INFINITY)); // No target or actor found
  }

  return GoapBlackboardValue(this->selected_entity.get(ctx.game).position);
}

GoapBlackboardValue FlagStateSensor::sense(SmartAIThinkCtx &ctx) {
  auto flag_entity = flag_selector(ctx);

  if (!flag_entity.valid_as<FlagEntity>(ctx.game)) {
    return GoapBlackboardValue(false); // No flag found
  }

  auto &flag = flag_entity.get_as<FlagEntity>(ctx.game);

  return GoapBlackboardValue(holder_filter(ctx, flag.flag_holder));
}

GoapBlackboardValue HasLineOfSightSensor::sense(SmartAIThinkCtx &ctx) {
  auto target_entity = target_selector(ctx);

  if (!target_entity.valid(ctx.game)) {
    return GoapBlackboardValue(false); // No target found
  }

  const float CHECK_DIST = 35.0f;
  Vec2 target_pos = target_entity.get(ctx.game).position;
  Vec2 curr_pos = ctx.character.world_projectile_launch_pos();

  Vec2 dir = (target_pos - curr_pos).normalized();
  float dist = curr_pos.distance(target_pos);
  size_t check_count = 500;
  for (float i = 0.0f; i < dist; i += CHECK_DIST) {
    check_count--;
    if (check_count == 0) {
      return GoapBlackboardValue(
          false); // Too many checks, probably no line of sight
    }

    Vec2 check_pos = curr_pos + dir * i;

    for (auto &entity : ctx.game.valid_entities()) {
      if (TilemapEntity *tm = dynamic_cast<TilemapEntity *>(entity.get())) {

        if (tm->check_collision_point(check_pos) == TileType::SOLID) {

          return GoapBlackboardValue(false); // There is a solid tile in the way
        }
      }

      if (PhysEntity *phys_ent = dynamic_cast<PhysEntity *>(entity.get())) {
        if (!phys_ent->get_aabb()
                 .translated(phys_ent->position)
                 .contains(check_pos)) {
          continue;
        }
        if (phys_ent->net_id == ctx.character.net_id) {
          // Ignore ourselves
          continue;
        }
        if (phys_ent->net_id == target_entity.get(ctx.game).net_id) {
          // We are checking the target entity itself, so we can see it
          return GoapBlackboardValue(true);
        }
      }
    }
  }

  return GoapBlackboardValue(
      false); // No solid tiles in the way, we can see the target
}

void giewont::apply_gameplay_logic_to_predicted_blackboard(
    SmartAIThinkCtx &ctx, GoapBlackboard &blackboard) {
  if (blackboard.contains(GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG) &&
      std::get<bool>(blackboard[GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG]) ==
          true) {
    // If the character is holding the enemy flag, then it's position will be
    // the same as the character's position
    blackboard[GoapBlackboardKey::ENEMY_FLAG_POS] =
        std::get<Vec2>(blackboard[GoapBlackboardKey::OWN_POS]);

    blackboard[GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG] =
        true; // We are anybody
  }

  if (blackboard.contains(GoapBlackboardKey::CLOSEST_ENEMY_HEALTH_PERCENTAGE)) {

    auto closest_enemy_health = std::get<float>(
        blackboard[GoapBlackboardKey::CLOSEST_ENEMY_HEALTH_PERCENTAGE]);

    if (std::isfinite(closest_enemy_health) && closest_enemy_health <= 0.0f) {
      // If the closest enemy is dead, we can remove it from the blackboard
      blackboard[GoapBlackboardKey::CLOSEST_ENEMY_POS] =
          Vec2(INFINITY, INFINITY);
      blackboard[GoapBlackboardKey::CLOSEST_ENEMY_HEALTH_PERCENTAGE] = NAN;
      blackboard[GoapBlackboardKey::HAS_LINE_OF_SIGHT_TO_CLOSEST_ENEMY] =
          false; // No line of sight to a dead enemy
    }
  }
  // if (blackboard_pos_distance(blackboard, GoapBlackboardKey::ENEMY_FLAG_POS,
  //                             GoapBlackboardKey::OWN_BASE_POS) <
  //     ctx.game.get_gvar<float>(GVarType::BASE_ACTIVATION_DIST)) {
  //   // If the own flag is close enough to the own base, then we are holding it
  //    blackboard[GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG] = true;
  //   // The team base is counted as a "holder" of the flag during the capture animation
    
  //       blackboard[GoapBlackboardKey::IS_ENEMY_FLAG_IN_CAPTURED_ANIMATION] =
  //           true; // The flag is in the captured animation state
  // }
}

float giewont::blackboard_pos_distance(const GoapBlackboard &blackboard,
                                       GoapBlackboardKey key1,
                                       GoapBlackboardKey key2) {
  if (!blackboard.contains(key1) || !blackboard.contains(key2)) {
    return INFINITY; // One of the keys is not present in the blackboard
  }

  const auto &pos1 = std::get<Vec2>(blackboard.at(key1));
  const auto &pos2 = std::get<Vec2>(blackboard.at(key2));
  if (!pos1.isfinite() || !pos2.isfinite()) {
    return INFINITY; // One of the positions is invalid
  }
  return pos1.distance(pos2);
}
