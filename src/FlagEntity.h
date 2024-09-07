#ifndef FLAGENTITY_H_
#define FLAGENTITY_H_

#include "PhysEntity.h"

namespace giewont {
class FlagEntity : public PhysEntity {
public:
  FlagEntity(const nlohmann::json &data);
  const char *get_type_name() const override { return "FlagEntity"; }
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  int32_t get_z_index() override { return 500; }

private:
  std::string _texture_path;
  res_id _texture_id;
};
} // namespace giewont

#endif // FLAGENTITY_H_
