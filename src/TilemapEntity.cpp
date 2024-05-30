#include "TilemapEntity.h"

#include <filesystem>
#include <raylib.h>

#include <iostream>

using namespace giewont;

void TilemapEntity::load_assets(const Game &game) {

  this->tilemap_res_id = game.rm->load_json(tilemap_json_path);

  std::filesystem::path tilemap_dir =
      std::filesystem::path(tilemap_json_path).parent_path();

  auto tilemapData = game.rm->get_json(this->tilemap_res_id);
  // check if "tilesets" is present and has at least one item
  if (!tilemapData->contains("tilesets") ||
      (*tilemapData)["tilesets"].size() == 0) {
    throw std::runtime_error("Tilemap must have at least one tileset");
  }

  // get the path to the tileset image
  std::filesystem::path tilesetPath =
      tilemap_dir / (*tilemapData)["tilesets"][0]["source"];

  int first_gid = (*tilemapData)["tilesets"][0]["firstgid"];

  auto tileset_json_res_id = game.rm->load_json(tilesetPath.string());

  auto tilesetData = game.rm->get_json(tileset_json_res_id);

  // get the path to the tileset image
  std::filesystem::path texturePath = tilemap_dir / (*tilesetData)["image"];
  auto tileset_texture_res_id = game.rm->load_texture(texturePath.string());

  this->tileset_data = std::make_unique<TilesetData>(
      tileset_json_res_id, tileset_texture_res_id, first_gid);
  this->tileset_data->load_tileset_data(game);

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

void TilemapEntity::update(const Game &game) {}

void TilemapEntity::draw(const Game &game) {
  auto tex = game.rm->get_texture(this->tileset_data->texture_res_id);
  for (int y = 0; y < this->tilemap_height; y++) {
    for (int x = 0; x < this->tilemap_width; x++) {
      int tile_id = this->tilemap_data[y * this->tilemap_width + x];
      if (tile_id == 0) {
        continue;
      }
      this->tileset_data->draw_tile(game, tile_id, x, y);
    }
  }
}

/////// TilesetData

TilesetData::TilesetData(res_id tileset_res_id, res_id texture_res_id,
                         int first_gid) {
  this->tileset_res_id = tileset_res_id;
  this->texture_res_id = texture_res_id;
  this->first_gid = first_gid;
}

void TilesetData::load_tileset_data(const Game &game) {
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

void TilesetData::draw_tile(const Game &game, int tile_id, int pos_x,
                            int pos_y) {
  auto tex = game.rm->get_texture(this->texture_res_id);
  int tile_x = (tile_id - this->first_gid) % this->columns;
  int tile_y = (tile_id - this->first_gid) / this->columns;
  Rectangle src = {.x = (float)(tile_x * (this->tile_width + this->spacing)),
                   .y = (float)(tile_y * (this->tile_height + this->spacing)),
                   .width = (float)this->tile_width,
                   .height = (float)this->tile_height};
  Vector2 dest = {
      .x = (float)pos_x * this->tile_width,

      .y = (float)pos_y * this->tile_height,
  };
  DrawTextureRec(*tex, src, dest, WHITE);
}
