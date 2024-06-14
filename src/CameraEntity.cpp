#include "CameraEntity.h"

using namespace giewont;

CameraEntity::CameraEntity() {

#ifdef GIEWONT_HAS_GRAPHICS
  // Set up the camera
  camera.target = {0.0f, 0.0f};
  camera.offset = {50.0f, 50.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;
#endif
}

void CameraEntity::load_assets(const Game &game) {}

void CameraEntity::update(Game &game, float delta_time) {
  // check scroll
}

void CameraEntity::draw(const Game &game) {

#ifdef GIEWONT_HAS_GRAPHICS
  // Do nothing
  float mouse_scroll = GetMouseWheelMove();

  if (mouse_scroll != 0) {
    camera.zoom += mouse_scroll * 0.1f;
  }
#endif
}

#ifdef GIEWONT_HAS_GRAPHICS

void CameraEntity::begin_mode2d() { BeginMode2D(camera); }

void CameraEntity::end_mode2d() { EndMode2D(); }

#endif
