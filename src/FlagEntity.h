#ifndef FLAGENTITY_H_
#define FLAGENTITY_H_

#include "PhysEntity.h"

#include "DataBinder.h"
#include "math/Color.h"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

namespace giewont {

enum class FlagEntitySpriteType { POLE, WAVING, DEFORM_UP, DEFORM_DOWN };

class FLagEntitySprite {
public:
  int spritesheet_x;
  int spritesheet_y;
  int spritesheet_w;
  int spritesheet_h;
};

class FlagEntity : public PhysEntity {
public:
  FlagEntity();
  FlagEntity(nlohmann::json data);
  const char *get_type_name() const override { return "FlagEntity"; }
  net::EntityType get_net_type() override { return net::EntityType::FLAG; }
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  int32_t get_z_index() override { return 500; }
  AABB &get_aabb() override;

  GColor color = GColor(1.0f, 0.0f, 0.0f, 1.0f);

  EntityRef flag_holder;

  bool handle_interaction(
      Game &game, const net::InteractNetMessage::Reader interaction) override;

  GW_DATABINDER_DECLARE(FlagEntity);
  GW_DATABINDER_AUTO_INSPECTOR(klass);

private:
  std::unordered_map<FlagEntitySpriteType, std::vector<FLagEntitySprite>>
      sprites;
  std::string _texture_path;
  res_id _texture_id;
  res_id _spritesheet_data_id;
  AABB flag_aabb = AABB();

  void load_spritesheet_data(const Game &game);
};
} // namespace giewont

#endif // FLAGENTITY_H_
