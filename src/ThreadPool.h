#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace giewont {
class ThreadPool {
public:
  ThreadPool();

  void queue_job(const std::function<void()> &job);

  bool busy();
  ~ThreadPool();

private:
  void thread_loop();

  bool should_terminate = false; // Tells threads to stop looking for jobs
  std::mutex queue_mutex;        // Prevents data races to the job queue
  std::condition_variable
      mutex_condition; // Allows threads to wait on new jobs or termination
  std::vector<std::thread> threads;
  std::queue<std::function<void()>> jobs;
};

} // namespace giewont
