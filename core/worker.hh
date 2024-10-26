#pragma once

#include <atomic> 

#include "queue.hh"
#include "task.hh"

namespace vial {

class TaskBase;

class Worker {
  public:
    Worker(Queue<TaskBase*>*);
    void stop();
    void start();

  private:
    Queue<TaskBase*>* queue_;
    std::atomic<bool> running_ = true;
};

};