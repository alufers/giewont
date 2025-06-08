#include "GoapInterfaces.h"

using namespace giewont;

std::string giewont::goap_blackboard_key_to_string(GoapBlackboardKey key) {
  switch (key) {
    case GoapBlackboardKey::INVALID:
      return "INVALID";
    case GoapBlackboardKey::IS_PROPPED:
      return "IS_PROPPED";
    case GoapBlackboardKey::HEALTH_PERCENTAGE:
      return "HEALTH_PERCENTAGE";
    case GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG:
      return "IS_HOLDING_ENEMY_FLAG";
    case GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG:
      return "IS_ANYBODY_HOLDING_ENEMY_FLAG";
    case GoapBlackboardKey::OWN_POS:
      return "OWN_POS";
    case GoapBlackboardKey::ENEMY_FLAG_POS:
      return "ENEMY_FLAG_POS";
    case GoapBlackboardKey::ENEMY_BASE_POS:
      return "ENEMY_BASE_POS";
    case GoapBlackboardKey::OWN_FLAG_POS:
      return "OWN_FLAG_POS";
    case GoapBlackboardKey::OWN_BASE_POS:
      return "OWN_BASE_POS";
    case GoapBlackboardKey::CLOSEST_ENEMY_POS:
      return "CLOSEST_ENEMY_POS";
    case GoapBlackboardKey::CLOSEST_FRIENDLY_POS:
      return "CLOSEST_FRIENDLY_POS";
    case GoapBlackboardKey::IS_ANYBODY_HOLDING_OWN_FLAG:
      return "IS_ANYBODY_HOLDING_OWN_FLAG";
    case GoapBlackboardKey::OWN_POINTS:
      return "OWN_POINTS";
    case GoapBlackboardKey::UPDATES_SINCE_LAST_PATHFINDING:
      return "UPDATES_SINCE_LAST_PATHFINDING";
  }
  return "Unknown Key";
}

std::string
giewont::goap_blackboard_value_to_string(const GoapBlackboardValue &value) {
  return std::visit(
      [](const auto &v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>) {
          return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, float>) {
          return std::to_string(v);
        } else if constexpr (std::is_same_v<T, int>) {
          return std::to_string(v);
        } else if constexpr (std::is_same_v<T, Vec2>) {
          return "Vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) +
                 ")";
        } else if constexpr (std::is_same_v<T, EntityRef>) {
          return "EntityRef(id=" + std::to_string(v.id) +
                 ", generation=" + std::to_string(v.generation) + ")";
        } else {
          return "Unknown type";
        }
      },
      value);
}
