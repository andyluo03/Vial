#pragma once

#include <vector>
#include <thread>

#include "task.hh"
#include "queue.hh"

namespace vial {

constexpr size_t kMaxLocalTasks = 256;

class Scheduler {
  public:
    Scheduler(unsigned int num_workers = std::thread::hardware_concurrency());
    auto start () -> void;
    auto stop () -> void;

    auto push_task(TaskBase* task, size_t worker_id) -> void;

    template <typename T>
    void fire_and_forget(Task<T> task) {
      spawn_task(task);
    }

    template <typename T>
    auto spawn_task(Task<T> task) -> Task<T> {
      task.set_enqueued_true();
      global_queue_.push(task.clone());
      return task;
    }

  private:
    void run_worker (size_t worker_id);

    std::vector<std::queue<TaskBase*>> queues_;
    Queue<TaskBase*> global_queue_;
    
    bool running_ = false;
    size_t num_workers_;
};

};