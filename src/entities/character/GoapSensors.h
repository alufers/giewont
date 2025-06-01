#pragma once

#include "GoapInterfaces.h"
#include <memory>
#include <vector>
#include "GoapSelectors.h"

namespace giewont {

/// @brief Constructs a map of sensors which sense all the keys based on the
/// gameplay state.
std::vector<std::unique_ptr<GoapSensor>> construct_goap_sensors();

class IsProppedSensor : public GoapSensor {
public:
  GoapBlackboardKey get_key() const override {
    return GoapBlackboardKey::IS_PROPPED;
  }

  GoapBlackboardValue sense(SmartAIThinkCtx &ctx) override;

  ~IsProppedSensor() override = default;
};

class HealthPercentageSensor : public GoapSensor {
public:
  GoapBlackboardKey get_key() const override {
    return GoapBlackboardKey::HEALTH_PERCENTAGE;
  }

  GoapBlackboardValue sense(SmartAIThinkCtx &ctx);

  ~HealthPercentageSensor() override = default;
};

// Implements a sensor measuring the distance to various entities of interest.
class DistToSensor : public GoapSensor {
public:
  DistToSensor(GoapBlackboardKey key, GoapEntitySelectorFunc selector) : key(key), selector(selector) {}
  GoapBlackboardKey get_key() const override { return key; }
  GoapBlackboardValue sense(SmartAIThinkCtx &ctx);

  ~DistToSensor() override = default;

private:
  GoapBlackboardKey key;
  EntityRef closest_entity;
  GoapEntitySelectorFunc selector;


  uint32_t entity_scan_counter = 0;
};

class OwnFlagStateSensor : public GoapSensor {
public:
  OwnFlagStateSensor(GoapBlackboardKey key) : key(key) {}
  GoapBlackboardKey get_key() const override { return key; }
  GoapBlackboardValue sense(SmartAIThinkCtx &ctx) override;

  ~OwnFlagStateSensor() override = default;

private:
  GoapBlackboardKey key;
};

class EnemyFlagStateSensor : public GoapSensor {
public:
  EnemyFlagStateSensor(GoapBlackboardKey key) : key(key) {}
  GoapBlackboardKey get_key() const override { return key; }
  GoapBlackboardValue sense(SmartAIThinkCtx &ctx) override;

  ~EnemyFlagStateSensor() override = default;

private:
  GoapBlackboardKey key;
};

} // namespace giewont
