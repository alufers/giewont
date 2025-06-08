#pragma once

#include "GoapInterfaces.h"
#include "GoapSelectors.h"
#include <memory>
#include <vector>

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

  GoapBlackboardValue sense(SmartAIThinkCtx &ctx) override;

  ~HealthPercentageSensor() override = default;
};

// Implements a sensor which determines the position of various entities
class PositionSensor : public GoapSensor {
public:
  PositionSensor(GoapBlackboardKey key, GoapEntitySelectorFunc target_selector)
      : key(key), target_selector(target_selector) {}
  GoapBlackboardKey get_key() const override { return key; }
  GoapBlackboardValue sense(SmartAIThinkCtx &ctx) override;

  ~PositionSensor() override = default;

private:
  GoapBlackboardKey key;
  EntityRef selected_entity;
  GoapEntitySelectorFunc target_selector;

  uint32_t entity_scan_counter = 0;
};

class FlagStateSensor : public GoapSensor {
public:
  FlagStateSensor(GoapBlackboardKey key, GoapEntitySelectorFunc flag_selector, GoapEntityFilterFunc holder_filter) : key(key) , flag_selector(flag_selector), holder_filter(holder_filter) {}
  GoapBlackboardKey get_key() const override { return key; }
  GoapBlackboardValue sense(SmartAIThinkCtx &ctx) override;

  ~FlagStateSensor() override = default;

private:
  GoapBlackboardKey key;
  GoapEntitySelectorFunc flag_selector;
  GoapEntityFilterFunc holder_filter;
};



} // namespace giewont
