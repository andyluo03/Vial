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
      size_t num_workers = 8
    );

    void start();

    template <typename T>
    void fire_and_forget(Task<T>);

    template <typename T>
    Task<T> spawn_task(Task<T>);

    std::atomic<bool>* get_running();

    ~Scheduler();

  private:
    Queue<TaskBase*> queue_;
    Worker* worker_;

    std::vector<std::thread> worker_pool_;
    size_t num_workers_;

    std::atomic<bool> running_{true};
};

template <typename T>
void Scheduler::fire_and_forget (Task<T> x) {
    x.set_enqueued_true();
    queue_.enqueue(new Task<T>(x));
}

template <typename T>
Task<T> Scheduler::spawn_task (Task<T> x) {
  this->fire_and_forget(x);
  return x;
}



}