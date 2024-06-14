#ifndef VEC2_H_
#define VEC2_H_

#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif
#include <cmath>
#include <iostream>

namespace giewont {

class Vec2 {
public:
  float x;
  float y;
  Vec2(float x, float y) : x(x), y(y) {}

  Vec2 operator+(const Vec2 &other) const {
    return Vec2(x + other.x, y + other.y);
  }
  Vec2 operator-(const Vec2 &other) const {
    return Vec2(x - other.x, y - other.y);
  }
  Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
  Vec2 operator/(float scalar) const { return Vec2(x / scalar, y / scalar); }
  Vec2 operator+=(const Vec2 &other) {
    x += other.x;
    y += other.y;
    return *this;
  }
  Vec2 operator-=(const Vec2 &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }
  Vec2 operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
  }
  Vec2 operator/=(float scalar) {
    x /= scalar;
    y /= scalar;
    return *this;
  }
  bool operator==(const Vec2 &other) const {
    return x == other.x && y == other.y;
  }
  bool operator!=(const Vec2 &other) const {
    return x != other.x || y != other.y;
  }
  Vec2 operator-() const { return Vec2(-x, -y); }
  float length() const { return std::sqrt(x * x + y * y); }

  Vec2 normalized() const {
    float len = length();
    if (len == 0) {
      return Vec2(0, 0);
    }
    return *this / len;
  }

  Vec2 lerp(const Vec2 &other, float t) const {
    return *this * (1 - t) + other * t;
  }

  float dot(const Vec2 &other) const { return x * other.x + y * other.y; }
  Vec2 project(const Vec2 &other) const {
    return other * (dot(other) / other.dot(other));
  }
  
#ifdef GIEWONT_HAS_GRAPHICS
  Vector2 to_raylib() const { return {x, y}; }
#endif

  friend auto operator<<(std::ostream &os, const Vec2 &v) -> std::ostream & {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
  }
};
} // namespace giewont

#endif // VEC2_H_
