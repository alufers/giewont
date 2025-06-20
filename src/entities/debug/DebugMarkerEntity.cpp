#include "DebugMarkerEntity.h"

using namespace giewont;

DebugMarkerEntity::DebugMarkerEntity() : Entity() {
    this->is_static = true;
}

void DebugMarkerEntity::load_assets(const Game &game) {
  Entity::load_assets(game);
}

void DebugMarkerEntity::update(Game &game, float delta_time) {
  lifetime -= delta_time;
  if (lifetime <= 0.0f) {
    this->destroy();
  }
}

void DebugMarkerEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  DrawCircleV(position.to_raylib(), 5.0f, RED);
#endif
}
