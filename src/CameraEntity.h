#ifndef CAMERAENTITY_H_
#define CAMERAENTITY_H_

#include "Entity.h"
#include "Game.h"
#include "schema.capnp.h"
#include <memory>
#include <vector>
#include "Color.h"

#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif

namespace giewont {

class CameraEffect {
public:
  float duration = 0.0f;
  float duration_left = 0.0f;
  float intensity = 0.0f;
  float falloff_time = 0.0f;
  float speed = 0.0f;

  GColor color ;

#ifdef GIEWONT_HAS_GRAPHICS
  virtual Camera2D modify_camera(float delta_time, const Camera2D &input);
  virtual void on_before_begin_mode2d() {}
  virtual void on_after_end_mode2d() {}
#endif

protected:
  float computed_intensity();
};

class CameraShakeEffect : public CameraEffect {
public:
#ifdef GIEWONT_HAS_GRAPHICS
  Camera2D modify_camera(float delta_time, const Camera2D &input) override;
#endif
};

class VignetteEffect : public CameraEffect {
public:
#ifdef GIEWONT_HAS_GRAPHICS
  void on_after_end_mode2d() override;
#endif
};

/**
 * @brief Entity for managing the camera.
 * begin_mode2d and end_mode2d shall be called by the main draw loop once per
 * frame (before and after drawing all entities).
 */
class CameraEntity : public Entity {
public:
  CameraEntity();

  const char *get_type_name() const override { return "CameraEntity"; }

  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  void draw_inspector_ui(Game &game) override;

  void handle_add_camera_effect(const net::AddCameraEffectNetMessage::Reader &message);

  EntityRef entity_to_follow;
  std::vector<std::unique_ptr<CameraEffect>> effects;

#ifdef GIEWONT_HAS_GRAPHICS
  void begin_mode2d();
  void end_mode2d();
#endif

#ifdef GIEWONT_HAS_GRAPHICS
  Camera2D camera = {0};
  Camera2D camera_after_effects = {0};
#endif

private:
};

} // namespace giewont
#endif // CAMERAENTITY_H_
