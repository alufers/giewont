
#include "ThreadPool.h"

using namespace giewont;

ThreadPool::ThreadPool() {
  const uint32_t num_threads =
      std::thread::hardware_concurrency(); // Max # of threads the system
                                           // supports
  for (uint32_t ii = 0; ii < num_threads; ++ii) {
    threads.emplace_back(std::thread(&ThreadPool::thread_loop, this));
  }
}

void ThreadPool::thread_loop() {
  while (true) {
    std::function<void()> job;
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      mutex_condition.wait(
          lock, [this] { return !jobs.empty() || should_terminate; });
      if (should_terminate) {
        return;
      }
      job = jobs.front();
      jobs.pop();
    }
    job();
  }
}

void ThreadPool::queue_job(const std::function<void()> &job) {
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    jobs.push(job);
  }
  mutex_condition.notify_one();
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    should_terminate = true;
  }
  mutex_condition.notify_all();
  for (std::thread &active_thread : threads) {
    active_thread.join();
  }
  threads.clear();
}
