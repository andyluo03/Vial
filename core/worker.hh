#pragma once

#include <atomic> 

#include "queue.hh"
#include "task.hh"

namespace vial {

class TaskBase;

class Worker {
  public:
    Worker(Queue<TaskBase*>* queue, std::atomic<bool>* running);
    void stop();
    void start();
    void enqueue(TaskBase* task);

  private:
    Queue<TaskBase*>* queue_;
    std::atomic<bool>* running_;
};

};