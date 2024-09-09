#ifndef COLOR_H_
#define COLOR_H_

#include <cstdint>

#ifdef GIEWONT_HAS_GRAPHICS
#include "imgui.h"
#include "raylib.h"
#endif

namespace giewont {

class GColor {
public:
  float r;
  float g;
  float b;
  float a;
  GColor(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
  GColor(float r, float g, float b) : r(r), g(g), b(b), a(1.0f) {}
  GColor() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
  GColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
      : r(r / 255.0f), g(g / 255.0f), b(b / 255.0f), a(a / 255.0f) {}
  GColor(unsigned char r, unsigned char g, unsigned char b)
      : r(r / 255.0f), g(g / 255.0f), b(b / 255.0f), a(1.0f) {}

  uint32_t to_uint32() const {
    return (static_cast<uint32_t>(a * 255) << 24) |
           (static_cast<uint32_t>(b * 255) << 16) |
           (static_cast<uint32_t>(g * 255) << 8) |
           static_cast<uint32_t>(r * 255);
  }

  static GColor from_uint32(uint32_t color) {
    return GColor(
        (color & 0x000000FF) / 255.0f, ((color & 0x0000FF00) >> 8) / 255.0f,
        ((color & 0x00FF0000) >> 16) / 255.0f, (color >> 24) / 255.0f);
  }

#ifdef GIEWONT_HAS_GRAPHICS
  Color to_raylib_color() const {
    return {static_cast<unsigned char>(r * 255),
            static_cast<unsigned char>(g * 255),
            static_cast<unsigned char>(b * 255),
            static_cast<unsigned char>(a * 255)};
  }

  void editor_ui(const char *label) {
    float f[4] = {r, g, b, a};
    ImGui::ColorEdit4(label, f);
    r = f[0];
    g = f[1];
    b = f[2];
    a = f[3];
  }
#endif
};

} // namespace giewont

#endif // COLOR_H_
