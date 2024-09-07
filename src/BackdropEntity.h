#ifndef BACKDROPENTITY_H_
#define BACKDROPENTITY_H_

#include "Entity.h"
#include "ResourceManager.h"
#include "stdint.h"
#include <nlohmann/json.hpp>

#ifdef GIEWONT_HAS_GRAPHICS
#include "raylib.h"
#endif

namespace giewont {
class BackdropEntity : public Entity {
public:
  BackdropEntity(const nlohmann::json &data);
  const char *get_type_name() const override { return "BackdropEntity"; }
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;
  void draw_inspector_ui(Game &game) override;

  int32_t get_z_index() override { return -100; }

private:
  std::string _texture_path;
  res_id _texture_id;

  Vec2 parallaxFactor = {0.1f, 0.03f};
#ifdef GIEWONT_HAS_GRAPHICS
  Color top_color = {0, 0, 0, 255};
  Color bottom_color = {0, 0, 0, 255};
#endif
};
} // namespace giewont

#endif // BACKDROPENTITY_H_
