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

    template <typename T>
    void fire_and_forget(Task<T>);

    template <typename T>
    Task<T> spawn_task(Task<T>);

    std::atomic<bool>* get_running();

    ~Engine();

  private:
    Queue<TaskBase*> queue_;
    Worker* worker_;

    std::vector<std::thread> dispatcher_pool_;
    std::vector<std::thread> worker_pool_;

    size_t num_dispatchers_;
    size_t num_workers_;

    std::atomic<bool> running_{true};
};

template <typename T>
void Engine::fire_and_forget (Task<T> x) {
    queue_.enqueue(new Task<T>(x));
    x.set_enqueued();
}

template <typename T>
Task<T> Engine::spawn_task (Task<T> x) {
  this->fire_and_forget(x);
  return x;
}



}