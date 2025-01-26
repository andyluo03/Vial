#pragma once 

#include <thread>
#include <vector>

#include "queue.hh"
#include "worker.hh"
#include "task.hh"

namespace vial {

class Scheduler {
  public:
    Scheduler (
      size_t num_workers = std::thread::hardware_concurrency()
    );

    Scheduler(const Scheduler&) = delete;
    Scheduler(const Scheduler&&) = delete;
    auto operator=(const Scheduler&) -> Scheduler& = delete;
    auto operator=(const Scheduler&&) -> Scheduler& = delete;
    
    ~Scheduler();

    void start();

    template <typename T>
    void fire_and_forget(Task<T> task);

    template <typename T>
    auto spawn_task(Task<T> task) -> Task<T>;

    auto get_running() -> std::atomic<bool>*;
  private:
    Queue<TaskBase*> queue_;
    Worker* worker_ = nullptr;

    std::vector<std::thread> worker_pool_;
    size_t num_workers_;

    std::atomic<bool> running_{true};
};

template <typename T>
void Scheduler::fire_and_forget (Task<T> task) {
    task.set_enqueued_true();
    queue_.enqueue(new Task<T>(task));
}

template <typename T>
auto Scheduler::spawn_task (Task<T> task) -> Task<T> {
  this->fire_and_forget(task);
  return task;
}


}