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
        std::optional<TaskBase*> task_opt = queue_->try_get();

        // Minimize this.
        if (task_opt == std::nullopt) { 
            sched_yield();
            continue; // no available tasks
        }

        auto task = task_opt.value();

        if (task->get_awaiting() != nullptr && task->get_awaiting()->get_state() != TaskState::kComplete) {
            queue_->enqueue(task);
            continue;
        }

        TaskState state = task->run();

        switch ( state ) {
            // We want this to be == to time co_await is called.
            case TaskState::kAwaiting: {
                    auto awaiting_on = task->get_awaiting();
                    assert(awaiting_on != nullptr);

                    // Awaiting on hasn't been spawned yet. 
                    if (!awaiting_on->enqueued()) { 
                        task->set_enqueued_false();

                        awaiting_on->set_callback(task);
                        awaiting_on->set_enqueued_true();
                        queue_->enqueue(awaiting_on);
                    } 
                    // Awaiting on has been spawned already.
                    else {
                        // Race conditions (?)
                        queue_->enqueue(task);
                    }
            } break;
            case TaskState::kComplete: {
                // Push onto queue any callbacks. 
                auto callback = task->get_callback();

                // Enqueue callback (if exists).
                if (callback != nullptr && callback->enqueued() == false) {
                    callback->set_enqueued_true();
                    queue_->enqueue(callback);
                    continue;
                } 

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