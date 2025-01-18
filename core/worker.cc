#include "worker.hh"
#include "queue.hh"
#include "task.hh"
#include <iostream>

namespace vial {

Worker::Worker(Queue<TaskBase*>* queue) : queue_{queue}, running_{true} {}

void Worker::start() {
    while ( running_ ) {
        TaskBase* task = queue_->get();
        TaskState state = task->run();

        switch ( state ) {
            case TaskState::kAwaiting: {
                if ( task->awaiting() != nullptr ) {
                    queue_->enqueue(task->awaiting());
                }
            } break;
            case TaskState::kComplete: {
                if ( task->callback() != nullptr ) {
                    queue_->enqueue(task->callback());
                }
            } break;
        }
    }
};

void Worker::stop() {
    running_ = false;
}

void Worker::enqueue(TaskBase* x) {
    queue_->enqueue(x);
}

};