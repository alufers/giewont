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
  FlagStateSensor(GoapBlackboardKey key, GoapEntitySelectorFunc flag_selector,
                  GoapEntityFilterFunc holder_filter)
      : key(key), flag_selector(flag_selector), holder_filter(holder_filter) {}
  GoapBlackboardKey get_key() const override { return key; }
  GoapBlackboardValue sense(SmartAIThinkCtx &ctx) override;

  ~FlagStateSensor() override = default;

private:
  GoapBlackboardKey key;
  GoapEntitySelectorFunc flag_selector;
  GoapEntityFilterFunc holder_filter;
};

/**
 * @brief Given a predicted blackboard state (after some actions were performed
 * by the character), apply any gameplay logic external to the AI which might
 * change the sensors.
 *
 * For example, if the player has the flag, then update it's position in the
 * blackboard.
 *
 * @param ctx The context (used to extract gameplay variables)
 * @param blackboard The blackboard to apply the logic to
 */
void apply_gameplay_logic_to_predicted_blackboard(SmartAIThinkCtx &ctx,
                                                  GoapBlackboard &blackboard);

/**
 * @brief Given two blackboard *_POS keys, calculate the distance between them.
 *
 * @param blackboard The blackboard to extract the positions from
 * @param key1 The first position key
 * @param key2 The second position key
 * @return float  The distance between the two positions, or INFINITY if either
 * position is invalid.
 */
float blackboard_pos_distance(const GoapBlackboard &blackboard,
                              GoapBlackboardKey key1, GoapBlackboardKey key2);

                              
} // namespace giewont
