#include "GoapSensors.h"
#include "GoapInterfaces.h"
#include "SmartAIContainers.h"

using namespace giewont;

std::vector<std::unique_ptr<GoapSensor>> giewont::construct_goap_sensors() {
  std::vector<std::unique_ptr<GoapSensor>> sensors;

  sensors.push_back(std::make_unique<IsProppedSensor>());

  // Add other sensors here as needed
  return sensors;
}

GoapBlackboardValue IsProppedSensor::sense(SmartAIThinkCtx &ctx) {
  // Implement the logic to determine if the character is propped
  // For example, check the character's state or environment
  bool is_propped = ctx.character.is_propped_by_level;
  return GoapBlackboardValue(is_propped);
}
