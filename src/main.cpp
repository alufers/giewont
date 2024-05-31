#include "raylib.h"

#include "Game.h"
#include "TilemapEntity.h"
#include "PhysEntity.h"
#include <chrono>
#include <memory>

#define SCREEN_WIDTH (1920)
#define SCREEN_HEIGHT (1080)

#define WINDOW_TITLE "GIEWONT"

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(60);

  std::unique_ptr<giewont::Game> g = std::make_unique<giewont::Game>();
  g->push_entity(std::make_unique<giewont::TilemapEntity>("level1.tmj"));


  auto phys_ent = std::make_unique<giewont::PhysEntity>();
  phys_ent->position = {100, 100};
  phys_ent->velocity = {50.0f, 0};
  g->push_entity(std::move(phys_ent));

  // Load assets
  g->load_assets();

  std::chrono::time_point<std::chrono::system_clock> last_frame_time =
      std::chrono::system_clock::now();

  while (!WindowShouldClose()) {
    std::chrono::time_point<std::chrono::system_clock> current_time =
        std::chrono::system_clock::now();
    float delta_time =
        std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(
            current_time - last_frame_time)
            .count() /
        1000;

    g->update(delta_time);
    BeginDrawing();

    ClearBackground(RAYWHITE);
    g->draw();
    EndDrawing();

    last_frame_time = current_time;
  }

  CloseWindow();
  return 0;
}
