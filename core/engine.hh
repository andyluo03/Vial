#pragma once 

#include <thread>
#include <vector>

#include "queue.hh"
#include "worker.hh"
#include "task.hh"

namespace vial {

class Engine {
  public:
    Engine (
      size_t num_workers = std::max(static_cast<int>(std::thread::hardware_concurrency()) - 2, 2), 
      size_t num_dispatchers = 1
    );

    void start();

    void fire_and_forget(TaskBase*);

    ~Engine();

  private:
    Queue<TaskBase*> queue_;
    Worker* worker_;

    std::vector<std::thread> dispatcher_pool_;
    std::vector<std::thread> worker_pool_;

    size_t num_dispatchers_;
    size_t num_workers_;
};

}