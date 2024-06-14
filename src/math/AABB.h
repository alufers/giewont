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

  static AABB from_min_and_size(Vec2 min, Vec2 size) {
    return AABB(min, min + size);
  }

  bool contains(const Vec2 &point) const {
    return point.x >= min.x && point.x <= max.x && point.y >= min.y &&
           point.y <= max.y;
  }

  bool intersects(const AABB &other) const {
    return min.x <= other.max.x && max.x >= other.min.x &&
           min.y <= other.max.y && max.y >= other.min.y;
  }

  bool intersects_with_normal_penetration(const AABB &other, Vec2 &normal,
                                          float &penetration) const {

    if (!intersects(other)) {

      return false;
    }

    Vec2 n = other.center() - center();
    float a_extent_x = (max.x - min.x) / 2;
    float b_extent_x = (other.max.x - other.min.x) / 2;

    float x_overlap = a_extent_x + b_extent_x - std::abs(n.x);

    if (x_overlap > 0) {
      float a_extent_y = (max.y - min.y) / 2;
      float b_extent_y = (other.max.y - other.min.y) / 2;

      float y_overlap = a_extent_y + b_extent_y - std::abs(n.y);

      if (y_overlap > 0) {
        if (x_overlap < y_overlap) {
          if (n.x < 0) {
            normal = Vec2(-1, 0);
          } else {
            normal = Vec2(1, 0);
          }
          penetration = x_overlap;
        } else {
          if (n.y < 0) {
            normal = Vec2(0, -1);
          } else {
            normal = Vec2(0, 1);
          }
          penetration = y_overlap;
        }
        return true;
      }
    }

    return true;
  }

  Vec2 center() const { return (min + max) / 2; }

  Vec2 size() const { return max - min; }

  AABB translated(const Vec2 &offset) const {
    return AABB(min + offset, max + offset);
  }

  float width() const { return max.x - min.x; }
  float height() const { return max.y - min.y; }
};
} // namespace giewont

#endif // AABB_H_
