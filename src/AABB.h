#ifndef AABB_H_
#define AABB_H_

#include "Vec2.h"

namespace giewont {
class AABB {
public:
  Vec2 min;
  Vec2 max;

  AABB() : min(0, 0), max(0, 0) {}
  AABB(Vec2 min, Vec2 max) : min(min), max(max) {}

  bool contains(const Vec2 &point) const {
    return point.x >= min.x && point.x <= max.x && point.y >= min.y &&
           point.y <= max.y;
  }

  bool intersects(const AABB &other) const {
    return min.x <= other.max.x && max.x >= other.min.x &&
           min.y <= other.max.y && max.y >= other.min.y;
  }

  Vec2 center() const { return (min + max) / 2; }

  Vec2 size() const { return max - min; }

  AABB translated(const Vec2 &offset) const {
    return AABB(min + offset, max + offset);
  }
};
} // namespace giewont

#endif // AABB_H_
