#include "scheduler.hh"
#include "worker.hh"

namespace vial {

Scheduler::Scheduler (size_t num_workers)
    : worker_{new Worker(&queue_, &running_)}, num_workers_{num_workers} {}
    
void Scheduler::start () {
    for (int i = 0; i < num_workers_; i++) {
        worker_pool_.emplace_back(
            &Worker::start, worker_
        );
    }

    for (auto& i : worker_pool_) { i.join(); }
    worker_pool_.clear();
}

auto Scheduler::get_running() -> std::atomic<bool>* {
    return &running_;
}

Scheduler::~Scheduler() {
    delete worker_;
}

}