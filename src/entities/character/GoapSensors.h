#pragma once

#include "GoapInterfaces.h"
#include <vector>
#include <memory>

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



} // namespace giewont
