#ifndef PLAYERSPAWNENTITY_H_
#define PLAYERSPAWNENTITY_H_

#include "Entity.h"
#include <nlohmann/json.hpp>

namespace giewont {

class SpawnEntity : public Entity {
public:
  SpawnEntity(const nlohmann::json &data);
  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;

  std::string entity_type;

  bool did_spawn = false;

private:
  std::unique_ptr<Entity> construct_entity(const Game &game);
};

} // namespace giewont

#endif // PLAYERSPAWNENTITY_H_
