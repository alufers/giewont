#ifndef CAMERAENTITY_H_
#define CAMERAENTITY_H_

#include "Entity.h"
#include "Game.h"

#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif

namespace giewont {

/**
 * @brief Entity for managing the camera.
 * begin_mode2d and end_mode2d shall be called by the main draw loop once per
 * frame (before and after drawing all entities).
 */
class CameraEntity : public Entity {
public:
  CameraEntity();
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;
#ifdef GIEWONT_HAS_GRAPHICS
  void begin_mode2d();
  void end_mode2d();
#endif

private:
#ifdef GIEWONT_HAS_GRAPHICS
  Camera2D camera = {0};
#endif
};

} // namespace giewont
#endif // CAMERAENTITY_H_
