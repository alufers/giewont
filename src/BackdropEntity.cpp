#include "BackdropEntity.h"
#include "CameraEntity.h"
#include "Entity.h"
#include "Game.h"

#ifdef GIEWONT_HAS_GRAPHICS
#include <imgui.h>
#include <raylib.h>
#endif

using namespace giewont;

BackdropEntity::BackdropEntity(const nlohmann::json &data) {
  if (data.find("properties") != data.end()) {
    auto properties = data["properties"];
    for (auto &property : properties) {
      if (property["name"] == "texture_path") {
        _texture_path = property["value"];
      }
    }
  } else {
    throw std::runtime_error("BackdropEntity must have a 'properties' key.");
  }

  this->position = Vec2(data["x"], data["y"]);
}

void BackdropEntity::load_assets(const Game &game) {
  _texture_id = game.rm->load_texture(_texture_path);

#ifdef GIEWONT_HAS_GRAPHICS
  // Sample top and bottom colors from the texture
  auto tex = game.rm->get_texture(_texture_id);
  if (tex->width > 0 && tex->height > 0) {
    Image img = LoadImageFromTexture(*tex);
    top_color = GetImageColor(img, img.width / 2, 0);
    bottom_color = GetImageColor(img, img.width / 2, img.height - 1);
    UnloadImage(img);
  }
#endif
}

void BackdropEntity::update(Game &game, float delta_time) {
  (void)game;
  (void)delta_time;
}

void BackdropEntity::draw(const Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  auto tex = game.rm->get_texture(_texture_id);

  int tex_width = tex->width;

  Camera2D camera = game.get_currently_rendering_camera_data();
  size_t tile_amount = 10;
  int strip_width = tex_width * tile_amount;

  float x_parallax_offset =
      -((int)(camera.target.x * this->parallaxFactor.x)) % (tex_width);
  float y_parallax_offset =
      -((int)(camera.target.y * this->parallaxFactor.y)) % (tex->height);

  int posX = camera.target.x - camera.offset.x + x_parallax_offset -
             ((float)strip_width / 4.0);
  int posY = this->position.y + y_parallax_offset;

  for (size_t i = 0; i < tile_amount; i++) {

    DrawTexture(*tex, tex_width * i + posX, posY, WHITE);
  }

  DrawRectangle(posX, posY - tex->height * 5, strip_width, tex->height * 5,
                top_color);

  DrawRectangle(posX, posY + tex->height, strip_width, tex->height * 6,
                bottom_color);

#endif
}

void BackdropEntity::draw_inspector_ui(Game &game) {
#ifdef GIEWONT_HAS_GRAPHICS
  Entity::draw_inspector_ui(game);
  ImGui::BeginGroup();
  ImGui::SliderFloat("X para", &parallaxFactor.x, 0.0f, 1.0f);

  ImGui::SliderFloat("Y para", &parallaxFactor.y, 0.0f, 1.0f);
  ImGui::EndGroup();
#endif
}
