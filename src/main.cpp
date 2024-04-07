#include "raylib.h"

#include "Game.h"
#include <chrono>
#include <memory>

#define SCREEN_WIDTH (800)
#define SCREEN_HEIGHT (450)

#define WINDOW_TITLE "GIEWONT"

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(60);

  std::unique_ptr<giewont::Game> g = std::make_unique<giewont::Game>();

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
