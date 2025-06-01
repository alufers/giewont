#include "GoapSensors.h"
#include "FlagEntity.h"
#include "GoapInterfaces.h"
#include "Log.h"
#include "SmartAIContainers.h"
#include "TeamBaseEntity.h"
using namespace giewont;

std::vector<std::unique_ptr<GoapSensor>> giewont::construct_goap_sensors() {
  std::vector<std::unique_ptr<GoapSensor>> sensors;

  sensors.push_back(std::make_unique<IsProppedSensor>());
  sensors.push_back(std::make_unique<HealthPercentageSensor>());

  sensors.push_back(std::make_unique<DistToSensor>(
      GoapBlackboardKey::DIST_TO_CLOSEST_ENEMY,
      goap_selectors::closest_selector(goap_selectors::is_enemy_character)));
  sensors.push_back(std::make_unique<DistToSensor>(
      GoapBlackboardKey::DIST_TO_CLOSEST_FRIENDLY,
      goap_selectors::closest_selector(goap_selectors::is_friendly_character)));
  sensors.push_back(std::make_unique<DistToSensor>(
      GoapBlackboardKey::DIST_TO_ENEMY_FLAG,
      goap_selectors::closest_selector(goap_selectors::is_enemy_flag)));
  sensors.push_back(std::make_unique<DistToSensor>(
      GoapBlackboardKey::DIST_TO_ENEMY_BASE,
      goap_selectors::closest_selector(goap_selectors::is_enemy_base)));
  sensors.push_back(std::make_unique<DistToSensor>(
      GoapBlackboardKey::DIST_TO_OWN_FLAG,
      goap_selectors::closest_selector(goap_selectors::is_friendly_flag)));
  sensors.push_back(std::make_unique<DistToSensor>(
      GoapBlackboardKey::DIST_TO_OWN_BASE,
      goap_selectors::closest_selector(goap_selectors::is_friendly_base)));

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

GoapBlackboardValue DistToSensor::sense(SmartAIThinkCtx &ctx) {
  if (!this->closest_entity.valid(ctx.game) ||
      (entity_scan_counter % 20) == 0) {
    this->closest_entity = selector(ctx);
  }
  entity_scan_counter++;
  if (!this->closest_entity.valid(ctx.game)) {
    return GoapBlackboardValue(INFINITY); // No target found
  }

  Vec2 target_pos = this->closest_entity.get(ctx.game).position;
  Vec2 character_pos = ctx.character.position;
  float distance = (target_pos - character_pos).length();
  return GoapBlackboardValue(distance);
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
