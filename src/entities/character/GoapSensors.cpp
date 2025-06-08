#include "GoapSensors.h"
#include "FlagEntity.h"
#include "GoapInterfaces.h"
#include "Log.h"
#include "SmartAIContainers.h"
#include "TeamBaseEntity.h"
using namespace giewont;
using namespace giewont::goap_selectors;

std::vector<std::unique_ptr<GoapSensor>> giewont::construct_goap_sensors() {
  std::vector<std::unique_ptr<GoapSensor>> sensors;

  sensors.push_back(std::make_unique<IsProppedSensor>());
  sensors.push_back(std::make_unique<HealthPercentageSensor>());

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

  sensors.push_back(std::make_unique<OwnFlagStateSensor>(
      GoapBlackboardKey::IS_ANYBODY_HOLDING_OWN_FLAG));
  sensors.push_back(std::make_unique<OwnFlagStateSensor>(
      GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG));
  sensors.push_back(std::make_unique<EnemyFlagStateSensor>(
      GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG));

  // Add other sensors here as needed
  return sensors;
}

GoapBlackboardValue IsProppedSensor::sense(SmartAIThinkCtx &ctx) {

  bool is_propped = ctx.character.is_propped_by_level;
  return GoapBlackboardValue(is_propped);
}

GoapBlackboardValue HealthPercentageSensor::sense(SmartAIThinkCtx &ctx) {
  float health_percentage = static_cast<float>(ctx.character.health) /
                            static_cast<float>(ctx.character.max_health);
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

GoapBlackboardValue OwnFlagStateSensor::sense(SmartAIThinkCtx &ctx) {
  for (auto &entity : ctx.game.valid_entities()) {
    if (FlagEntity *flag = dynamic_cast<FlagEntity *>(entity.get())) {
      if (flag->team == ctx.character.team) {
        if (key == GoapBlackboardKey::IS_ANYBODY_HOLDING_OWN_FLAG) {
          return GoapBlackboardValue(flag->flag_holder.valid(ctx.game));
        }
      }
    }
  }
  return GoapBlackboardValue(false); // No flag found for our team
}

GoapBlackboardValue EnemyFlagStateSensor::sense(SmartAIThinkCtx &ctx) {
  for (auto &entity : ctx.game.valid_entities()) {
    if (FlagEntity *flag = dynamic_cast<FlagEntity *>(entity.get())) {
      if (flag->team != ctx.character.team) {
        if (key == GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG) {
          return GoapBlackboardValue(flag->flag_holder.valid(ctx.game));
        } else if (key == GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG) {
          return GoapBlackboardValue(flag->flag_holder ==
                                     ctx.character.get_ref());
        }
      }
    }
  }
  return GoapBlackboardValue(INFINITY); // No flag found for enemy team
}
