#include "CameraEntity.h"

using namespace giewont;

CameraEntity::CameraEntity() {
  // Set up the camera
  camera.target = {0.0f, 0.0f};
  camera.offset = {50.0f, 50.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;
}

void CameraEntity::load_assets(const Game &game) {}

void CameraEntity::update(const Game &game, float delta_time) {
 
}

void CameraEntity::draw(const Game &game) {
  // Do nothing
}

void CameraEntity::begin_mode2d() { BeginMode2D(camera); }

void CameraEntity::end_mode2d() { EndMode2D(); }
