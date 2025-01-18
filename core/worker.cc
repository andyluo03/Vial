#include "worker.hh"
#include "queue.hh"
#include "task.hh"
#include <iostream>

namespace vial {

Worker::Worker(Queue<TaskBase*>* queue) : queue_{queue}, running_{true} {}

void Worker::start() {
    while ( running_ ) {
        // Ownership is transferred from queue_ to task. 
        TaskBase* task = queue_->get();
        TaskState state = task->run();
        
        switch ( state ) {
            case TaskState::kAwaiting: {
                if ( task->awaiting() != nullptr ) {
                    queue_->enqueue(task->awaiting());
                }
            } break;
            case TaskState::kComplete: {
                // Ownership of the callbacks is transferred to the queue. 
                for ( auto i : task->callbacks() ) {
                    queue_->enqueue(i);
                }

                //task->destroy() //unnecessary as successful completion *should* destroy the underlying coroutine.
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