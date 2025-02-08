#include "scheduler.hh"
#include "core/task.hh"
#include <thread>
#include <cassert>
#include <set>

namespace vial {

Scheduler::Scheduler(unsigned int num_workers) : num_workers_(num_workers) {
    queues_ = std::vector<std::queue<TaskBase*>>(num_workers_);
}

auto Scheduler::push_task(TaskBase* task, size_t worker_id) -> void {
    task->set_enqueued_true();
    if (queues_[worker_id].size() > kMaxLocalTasks) {
        queues_[worker_id].push(task);
    } else {
        global_queue_.push(task);
    }
}

auto Scheduler::start () -> void {
    running_ = true;
    std::vector<std::thread> workers;

    for (size_t i = 0; i < num_workers_; i++) {
        workers.emplace_back(
            &Scheduler::run_worker, this, i
        );
    }

    for (auto& i : workers) { i.join(); }

    workers.clear();
}

auto Scheduler::stop () -> void {
    running_ = false;
}

void Scheduler::run_worker(size_t worker_id) {
    std::set<void*> poop;

    auto& local_queue = queues_[worker_id];
    while (running_) {
        std::optional<TaskBase*> task_opt = local_queue.empty() ? std::nullopt : std::optional(local_queue.front());
        
        if(task_opt != std::nullopt) { local_queue.pop(); }

        while (task_opt == std::nullopt && running_) { task_opt = global_queue_.try_get(); }

        if (task_opt == std::nullopt) { continue; }

        TaskBase* task = task_opt.value();

        if (task->get_state() == kComplete) {
            this->push_task(task->get_callback() != nullptr ? task->get_callback() : task, worker_id);
            continue;
        }

        TaskBase* to_delete = task->get_awaiting();
        task->clear_awaiting();

        TaskState state = task->run();

        if (to_delete != nullptr) {
            to_delete->destroy();
        }

        delete to_delete;

        switch (state) {
            case kAwaiting: {
                auto *awaiting = task->get_awaiting();
                awaiting->set_callback(task);

                // Awaiting wasn't spawned. 
                if (!awaiting->is_enqueued()) {
                    this->push_task(awaiting, worker_id);
                }
            } break;

            case kComplete: {
                // On completion, a task should either push its callback or return the queue to wait for a callback.
                this->push_task(task->get_callback() != nullptr ? task->get_callback() : task, worker_id);
            } break;
        }
    }
}

};