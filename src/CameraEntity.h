#ifndef CAMERAENTITY_H_
#define CAMERAENTITY_H_

#include "Entity.h"
#include "Game.h"
#include "raylib.h"

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

  void begin_mode2d();
  void end_mode2d();

private:
  Camera2D camera = {0};
};

} // namespace giewont
#endif // CAMERAENTITY_H_
