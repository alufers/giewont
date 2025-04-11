#pragma once
#include <cmath>
#include <ostream>

namespace giewont {

class IVec2 {
public:
  int x;
  int y;
  IVec2(int x, int y) : x(x), y(y) {}

  IVec2 operator+(const IVec2 &other) const {
    return IVec2(x + other.x, y + other.y);
  }
  IVec2 operator-(const IVec2 &other) const {
    return IVec2(x - other.x, y - other.y);
  }

  bool operator==(const IVec2 &other) const {
    return x == other.x && y == other.y;
  }

  float length() const { return std::sqrt(static_cast<float>(x * x + y * y)); }

  float length2() const { return static_cast<float>(x * x + y * y); }

  friend auto operator<<(std::ostream &os, const IVec2 &v) -> std::ostream & {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
  }
};

} // namespace giewont
