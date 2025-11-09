#include "CameraEntity.h"

#include "Color.h"
#include "Entity.h"
#include "Log.h"
#include <algorithm>
#ifdef GIEWONT_HAS_GRAPHICS
#include "imgui.h"
#include "raylib.h"

#endif
using namespace giewont;

CameraEntity::CameraEntity() {

#ifdef GIEWONT_HAS_GRAPHICS
  // Set up the camera
  camera.target = {0.0f, 0.0f};
  camera.offset = {50.0f, 50.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.00f;
#endif
}

void CameraEntity::load_assets(const Game &game) {}

void CameraEntity::update(Game &game, float delta_time) {
#ifdef GIEWONT_HAS_GRAPHICS
  if (entity_to_follow.valid(game)) {

    auto &entity = entity_to_follow.get_as<Entity>(game);
    camera.target = entity.position.to_raylib();
    camera.offset = {(float)(GetScreenWidth() / 2.0),
                     (float)(GetScreenHeight() / 2.0)};
  }

  this->camera_after_effects = camera;
  for (auto it = effects.begin(); it != effects.end();) {
    auto &effect = *it;
    effect->duration_left -= delta_time;
    if (effect->duration_left <= 0) {
      it = effects.erase(it);
    } else {
      camera_after_effects =
          effect->modify_camera(delta_time, camera_after_effects);
      ++it;
    }
  }
#endif
}

void CameraEntity::draw(const Game &game) {

#ifdef GIEWONT_HAS_GRAPHICS
  // Do nothing
  float mouse_scroll = GetMouseWheelMove();

  if (mouse_scroll != 0) {
    camera.zoom += mouse_scroll * 0.1f;
    if(camera.zoom < 0.1f) {
      camera.zoom = 0.4f; // Prevent zooming out too much
    }
    if(camera.zoom > 10.0f) {
      camera.zoom = 10.0f; // Prevent zooming in too much
    }
  }
#endif
}

void CameraEntity::draw_inspector_ui(Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  Entity::draw_inspector_ui(game);
  ImGui::Text("Target: %f, %f", camera.target.x, camera.target.y);
  ImGui::Text("Offset: %f, %f", camera.offset.x, camera.offset.y);
  ImGui::Text("Rotation: %f", camera.rotation);
  ImGui::Text("Zoom: %f", camera.zoom);
  ImGui::Text("Effects: %zu", effects.size());

  ImGui::Separator();
  if (ImGui::Button("Add Shake Effect")) {
    auto effect = std::make_unique<CameraShakeEffect>();
    effect->duration = 0.5f;
    effect->duration_left = 0.5f;
    effect->intensity = 20.0f;
    effect->falloff_time = 0.2f;
    effect->speed = 150.0;
    effects.push_back(std::move(effect));
  }
  if (ImGui::Button("Add Vignette Effect")) {
    auto effect = std::make_unique<VignetteEffect>();
    effect->duration = 0.5f;
    effect->duration_left = 0.5f;
    effect->intensity = 0.5f;
    effect->falloff_time = 0.2f;
    effect->color = GColor(0.0f, 0.0f, 0.0f);

    effects.push_back(std::move(effect));
  }
#endif
}

void CameraEntity::handle_add_camera_effect(
    const net::AddCameraEffectNetMessage::Reader &message) {
#ifdef GIEWONT_HAS_GRAPHICS
  std::unique_ptr<CameraEffect> effect;

  if (message.getClearOthers()) {
    effects.clear();
  }

  switch (message.getType()) {
  case net::CameraEffectType::SHAKE:
    effect = std::make_unique<CameraEffect>();
    break;
  case net::CameraEffectType::VIGNETTE:
    effect = std::make_unique<VignetteEffect>();
    break;
  default:
    LOG_ERROR() << "Unknown camera effect type: " << (int)message.getType()
                << std::endl;
    return;
  }

  effect->duration = message.getDuration();
  effect->duration_left = message.getDuration();
  effect->intensity = message.getIntensity();
  effect->falloff_time = message.getFalloffDuration();
  effect->speed = message.getSpeed();
  effect->color = GColor::from_uint32(message.getColor());

  effects.push_back(std::move(effect));

#endif
}

#ifdef GIEWONT_HAS_GRAPHICS

void CameraEntity::begin_mode2d() {

  for (auto &effect : effects) {
    effect->on_before_begin_mode2d();
  }

  BeginMode2D(camera_after_effects);
}

void CameraEntity::end_mode2d() {
  EndMode2D();

  for (auto &effect : effects) {
    effect->on_after_end_mode2d();
  }
}

float CameraEffect::computed_intensity() {
  if (duration_left > falloff_time) {
    return intensity;
  }
  return std::clamp(intensity * (duration_left / falloff_time), 0.0f, intensity);
}

Camera2D CameraEffect::modify_camera(float delta_time, const Camera2D &input) {
  return input;
}

Camera2D CameraShakeEffect::modify_camera(float delta_time,
                                          const Camera2D &input) {
  if (duration_left > 0) {
    auto intensity = computed_intensity();
    float shake_x = sinf((duration - duration_left) * speed) * intensity;
    float shake_y =
        cosf((duration - duration_left) * speed + 30.4f) * intensity;

    Camera2D result = input;
    result.target.x += shake_x;
    result.target.y += shake_y;
    return result;
  }
  return input;
}

void VignetteEffect::on_after_end_mode2d() {

  float radius = std::sqrt(GetScreenWidth() * GetScreenWidth() +
                           GetScreenHeight() * GetScreenHeight()) *
                 0.5f;

  uint8_t alpha = (uint8_t)(computed_intensity() * 255);

  Color raylib_color = color.to_raylib_color();

  DrawCircleGradient(
      GetScreenWidth() / 2, GetScreenHeight() / 2, radius,
      Color{raylib_color.r, raylib_color.g, raylib_color.b, 0},
      Color{raylib_color.r, raylib_color.g, raylib_color.b, alpha});
}

#ifdef GIEWONT_HAS_GRAPHICS

Vec2 CameraEntity::screenToWorldPos(Vector2 raylib_pos) {
  Vector2 world_pos = GetScreenToWorld2D(raylib_pos, camera_after_effects);
  return Vec2(world_pos);
}

#endif

#endif
