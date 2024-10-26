#pragma once

#include "task.hh"
#include "queue.hh"

#include <atomic>
#include <iostream>

namespace vial {

class Dispatcher {
  public:
    virtual TaskBase* dispatch() = 0;
};

}