#include "TilemapEntity.h"

#include <filesystem>
#include <raylib.h>

#include "Log.h"
#include "Vec2.h"
#include <cmath>
#include <iostream>

using namespace giewont;

void TilemapEntity::load_assets(const Game &game) {

  this->tilemap_res_id = game.rm->load_json(tilemap_json_path);

  std::filesystem::path tilemap_dir =
      std::filesystem::path(tilemap_json_path).parent_path();

  auto tilemapData = game.rm->get_json(this->tilemap_res_id);

  // check if "tileheight" and "tilewidth" are present and are positive
  if (!tilemapData->contains("tileheight") ||
      !tilemapData->contains("tilewidth") ||
      (*tilemapData)["tileheight"] <= 0 || (*tilemapData)["tilewidth"] <= 0) {
    throw std::runtime_error(
        "Tilemap must have positive tileheight and tilewidth");
  }

  this->tile_size.x = (*tilemapData)["tilewidth"];
  this->tile_size.y = (*tilemapData)["tileheight"];

  // check if "tilesets" is present and has at least one item
  if (!tilemapData->contains("tilesets") ||
      (*tilemapData)["tilesets"].size() == 0) {
    throw std::runtime_error("Tilemap must have at least one tileset");
  }

  for (auto &tileset : (*tilemapData)["tilesets"]) {
    int first_gid = tileset["firstgid"];
    std::filesystem::path tilesetPath = tilemap_dir / tileset["source"];
    auto tileset_json_res_id = game.rm->load_json(tilesetPath.string());
    auto tilesetData = game.rm->get_json(tileset_json_res_id);
    std::filesystem::path texturePath = tilemap_dir / (*tilesetData)["image"];
    auto tileset_texture_res_id = game.rm->load_texture(texturePath.string());
    this->tilesets.push_back(
        TilesetData(tileset_json_res_id, tileset_texture_res_id, first_gid));
    this->tilesets.back().load_tileset_data(game);
  }

  for (auto &layer : (*tilemapData)["layers"]) {
    if (layer["type"] == "tilelayer") {
      this->tilemap_width = layer["width"];
      this->tilemap_height = layer["height"];
      for (int i = 0; i < this->tilemap_width * this->tilemap_height; i++) {
        int tile_id = layer["data"][i];
        this->tilemap_data.push_back(tile_id);
      }
    }
  }
  if (this->tilemap_data.size() != this->tilemap_width * this->tilemap_height) {
    throw std::runtime_error("Tilemap data size does not match width*height");
  }
}

TilemapEntity::TilemapEntity(std::string tilemap_json_path) {
  this->tilemap_json_path = tilemap_json_path;
};

void TilemapEntity::update(const Game &game, float delta_time) {}

void TilemapEntity::draw(const Game &game) {

  for (int y = 0; y < this->tilemap_height; y++) {
    for (int x = 0; x < this->tilemap_width; x++) {
      int tile_id = this->tilemap_data[y * this->tilemap_width + x];
      if (tile_id == 0) {
        continue;
      }
      this->get_tileset_for_tile_id(tile_id).draw_tile(
          game, tile_id, x, y, this->tile_size, this->position);
    }
  }
}

TilesetData &TilemapEntity::get_tileset_for_tile_id(int tile_id) {
  for (auto &tileset : this->tilesets) {
    if (tile_id >= tileset.first_gid &&
        tile_id < tileset.first_gid + tileset.tile_info.size()) {
      return tileset;
    }
  }
  throw std::runtime_error("Tileset not found for tile id: " +
                           std::to_string(tile_id));
}

TilesetTileInfo &TilemapEntity::get_tile_info_for_tile_id(int tile_id) {
  TilesetData &tileset = get_tileset_for_tile_id(tile_id);
  return tileset.tile_info[tile_id - tileset.first_gid];
}

bool TilemapEntity::check_collision_aabb(const AABB &aabb, Vec2 &resolution) {
  AABB in_local_space = aabb.translated(-this->position);

  AABB tilemap_aabb =
      AABB(Vec2(0, 0), Vec2((float)this->tilemap_width * this->tile_size.x,
                            (float)this->tilemap_height * this->tile_size.y));
  if (!tilemap_aabb.intersects(in_local_space)) {
    return false;
  }

  // tile coords
  int start_x = (int)in_local_space.min.x / this->tile_size.x;
  int start_y = (int)in_local_space.min.y / this->tile_size.y;
  int end_x = (int)std::ceil(in_local_space.max.x / this->tile_size.x);
  int end_y = (int)std::ceil(in_local_space.max.y / this->tile_size.y);

  for (int y = start_y; y < end_y; y++) {
    for (int x = start_x; x < end_x; x++) {
      if (x < 0 || y < 0 || x >= this->tilemap_width ||
          y >= this->tilemap_height) {
        continue;
      }
      int tile_id = this->tilemap_data[y * this->tilemap_width + x];
      if (tile_id == 0) { // short circuit for air tiles
        continue;
      }
      TilesetTileInfo &tile_info = get_tile_info_for_tile_id(tile_id);
      if (tile_info.tile == TileType::SOLID) {
        AABB tile_aabb = AABB::from_min_and_size(
            Vec2((float)x * this->tile_size.x, (float)y * this->tile_size.y),
            this->tile_size);
        if (tile_aabb.intersects_with_resolutions(in_local_space, resolution)) {
          return true;
        }
      }
    }
  }

  return false;
}

/////// TilesetData

TilesetData::TilesetData(res_id tileset_res_id, res_id texture_res_id,
                         int first_gid) {

  if (first_gid == 0) {
    throw std::runtime_error(
        "Cannot create tileset data non-air with first_gid == 0");
  }
  this->tileset_res_id = tileset_res_id;
  this->texture_res_id = texture_res_id;
  this->first_gid = first_gid;
}

TilesetData::TilesetData(int first_gid) {
  if (first_gid != 0) {
    throw std::runtime_error(
        "Cannot create tileset data for air with first_gid != 0");
  }
  this->first_gid = first_gid;
  this->tile_width = 0;
  this->tile_height = 0;
  this->spacing = 0;
  this->columns = 0;
  TilesetTileInfo air_info;
  air_info.tile = TileType::AIR;
  tile_info.push_back(air_info);
}

void TilesetData::load_tileset_data(const Game &game) {
  if (this->first_gid == 0) {
    return;
  }
  auto tileset_json = game.rm->get_json(this->tileset_res_id);

  this->tile_width = (*tileset_json)["tilewidth"];
  this->tile_height = (*tileset_json)["tileheight"];
  this->spacing = (*tileset_json)["spacing"];
  this->columns = (*tileset_json)["columns"];
  int tile_count = (*tileset_json)["tilecount"];
  for (int i = 0; i < tile_count; i++) {
    TilesetTileInfo info;
    info.tile = TileType::SOLID;
    tile_info.push_back(info);
  }

  for (auto &tile : (*tileset_json)["tiles"]) {
    int id = tile["id"];
    if (tile.contains("type")) {
      if (tile["type"] == "ladder") {
        tile_info[id].tile = TileType::LADDER;
      }
    }
  }
}

void TilesetData::draw_tile(const Game &game, int tile_id, int pos_x, int pos_y,
                            Vec2 size, Vec2 offset) {
  if (tile_id < this->first_gid) {
    return;
  }
  auto tex = game.rm->get_texture(this->texture_res_id);
  int tile_x = (tile_id - this->first_gid) % this->columns;
  int tile_y = (tile_id - this->first_gid) / this->columns;
  Rectangle src = {.x = (float)(tile_x * (this->tile_width + this->spacing)),
                   .y = (float)(tile_y * (this->tile_height + this->spacing)),
                   .width = (float)this->tile_width,
                   .height = (float)this->tile_height};
  Rectangle dest = {.x = (float)pos_x * this->tile_width + offset.x,

                    .y = (float)pos_y * this->tile_height + offset.y,
                    .width = size.x,
                    .height = size.y};

  Vector2 origin = {0, 0};

  DrawTexturePro(*tex, src, dest, origin, 0, WHITE);
}
