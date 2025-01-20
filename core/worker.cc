#include "worker.hh"
#include "queue.hh"
#include "task.hh"

namespace vial {

Worker::Worker(Queue<TaskBase*>* queue) : queue_{queue}, running_{true} {}

//! Start a worker thread. 
/*! 


Life-cycle of a Task.
*/
void Worker::start() {
    while ( running_ ) {
        // Ownership is transferred from queue_ to task. 
        TaskBase* task = queue_->get();

        if (task->awaiting() != nullptr && task->awaiting()->get_state() != TaskState::kComplete) {
            queue_->enqueue(task);
            continue;
        }
 
        TaskState state = task->run();

        switch ( state ) {
            case TaskState::kAwaiting: {
                    // Check if the awaiting task is queued. 
                    if (!task->awaiting()->enqueued()) {
                        queue_->enqueue(task->awaiting());
                        task->awaiting()->set_enqueued();
                    }

                    queue_->enqueue(task);
            } break;
            case TaskState::kComplete: {
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