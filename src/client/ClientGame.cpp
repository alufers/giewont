#include "ClientGame.h"
#include "CameraEntity.h"
#include <format>
#include "Log.h"

using namespace giewont;





void ClientGame::draw() {
  auto &camera = camera_ref.get_as<CameraEntity>(*this);
  camera.begin_mode2d();
  for (auto &entity : entities) {
    if (entity != nullptr) {
      entity->draw(*this);
    }
  }

  if (debug_overlay) {
    for (auto &entity : entities) {
      if (entity != nullptr) {
        entity->draw_debug(*this);
      }
    }
  }

  camera.end_mode2d();

  DrawText(std::format("UPS: {:.2f}", last_ups).c_str(), 10, 10, 20, BLACK);
}
