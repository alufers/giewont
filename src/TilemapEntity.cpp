#include "TilemapEntity.h"

using namespace giewont;

void TilemapEntity::load_assets(const Game &game) {
  this->tileset_res_id = game.rm->load_json("assets/tileset_base.tsj");
  this->tilemap_res_id = game.rm->load_json("assets/level1.tsj");
}

TilemapEntity::TilemapEntity(std::string tilemap_json_path) {
    this->tilemap_json_path = tilemap_json_path;
};


void TilemapEntity::update(const Game &game) {
 
}
