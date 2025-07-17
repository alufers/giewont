#pragma once
#include "Game.h"
#include "GameplayManager.h"
#include "ParticleSystemEntity.h"
#include "Entity.h"

namespace giewont {

class DebugMarkerEntity : public Entity {
public:
  DebugMarkerEntity();
  const char *get_type_name() const override { return "DebugMarkerEntity"; }

  void load_assets(const Game &game) override;
  void update(Game &game, float delta_time) override;
  void draw(const Game &game) override;


  
  // Synced variables
  float total_lifetime = 3.0f;
  float lifetime = total_lifetime;
 


};

} // namespace giewont
