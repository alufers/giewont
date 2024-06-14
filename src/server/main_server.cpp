#include "Log.h"
#include "ServerGame.h"
#include <chrono>
#include <iomanip>
#include <thread>

const float TARGET_UPS = 60.0f;

int main() {
  LOG_INFO() << "GIEWONT game server starting" << std::endl;
  std::unique_ptr<giewont::ServerGame> g =
      std::make_unique<giewont::ServerGame>("level1.tmj");

  g->init_net_server();



  
  std::chrono::time_point<std::chrono::system_clock> last_frame_time =
      std::chrono::system_clock::now();

  std::chrono::time_point<std::chrono::system_clock> last_info_print_time =
      std::chrono::system_clock::now();

  while (true) {

    std::chrono::time_point<std::chrono::system_clock> current_time =
        std::chrono::system_clock::now();
    float delta_time =
        std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(
            current_time - last_frame_time)
            .count() /
        1000;
    if (delta_time < 1.0f / TARGET_UPS) {
      std::this_thread::sleep_for(std::chrono::milliseconds(
          (int)((1.0f / TARGET_UPS - delta_time) * 1000)));
      current_time = std::chrono::system_clock::now();
      delta_time =
          std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(
              current_time - last_frame_time)
              .count() /
          1000;
    }

    g->update(delta_time);

    last_frame_time = current_time;

    if (std::chrono::duration_cast<std::chrono::seconds>(current_time -
                                                         last_info_print_time)
            .count() > 10) {

      LOG_INFO() << "Update rate: " << std::fixed << std::setprecision(2)
                 << 1.0 / delta_time << " Hz" << std::endl;
      last_info_print_time = current_time;
    }
  }
}
