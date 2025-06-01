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
  case GoapBlackboardKey::DIST_TO_ENEMY_FLAG:
    return "DIST_TO_ENEMY_FLAG";
  case GoapBlackboardKey::DIST_TO_ENEMY_BASE:
    return "DIST_TO_ENEMY_BASE";
  case GoapBlackboardKey::IS_HOLDING_ENEMY_FLAG:
    return "IS_HOLDING_ENEMY_FLAG";
  case GoapBlackboardKey::IS_ANYBODY_HOLDING_ENEMY_FLAG:
    return "IS_ANYBODY_HOLDING_ENEMY_FLAG";
  case GoapBlackboardKey::ENEMY_FLAG_DIST_TO_OWN_BASE:
    return "ENEMY_FLAG_DIST_TO_OWN_BASE";
  case GoapBlackboardKey::DIST_TO_CLOSEST_ENEMY:
    return "DIST_TO_CLOSEST_ENEMY";
  case GoapBlackboardKey::DIST_TO_CLOSEST_FRIENDLY:
    return "DIST_TO_CLOSEST_FRIENDLY";
  case GoapBlackboardKey::DIST_TO_OWN_FLAG:
    return "DIST_TO_OWN_FLAG";
  case GoapBlackboardKey::DIST_TO_OWN_BASE:
    return "DIST_TO_OWN_BASE";
  case GoapBlackboardKey::IS_ANYBODY_HOLDING_OWN_FLAG:
    return "IS_ANYBODY_HOLDING_OWN_FLAG";
  case GoapBlackboardKey::OWN_POINTS:
    return "OWN_POINTS";
  case GoapBlackboardKey::UPDATES_SINCE_LAST_PATHFINDING:
    return "UPDATES_SINCE_LAST_PATHFINDING";
  }
  return "UNKNOWN_KEY";
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
