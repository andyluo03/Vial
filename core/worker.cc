#include "worker.hh"
#include "queue.hh"
#include "task.hh"

namespace vial {

Worker::Worker(Queue<TaskBase*>* queue, std::atomic<bool>* running) : queue_{queue}, running_{running} {}

//! Start a worker thread. 
/*! 


Life-cycle of a Task.
*/
void Worker::start() {
    while ( *running_ ) {
        // Ownership is transferred from queue_ to task. 
        std::optional<TaskBase*> task_opt = queue_->get();

        if (task_opt == std::nullopt) { 
            sched_yield();
            continue; 
        }

        auto task = task_opt.value();

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
                //task->destroy();
            } break;
            case TaskState::kStop: {
                this->stop();
            } break;
        }
    }
};

void Worker::stop() {
    *running_ = false;
}

void Worker::enqueue(TaskBase* x) {
    queue_->enqueue(x);
}

};