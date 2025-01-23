#include "engine.hh"
#include "worker.hh"

namespace vial {

Scheduler::Scheduler (size_t num_workers)
    : num_workers_{num_workers} {}
    
void Scheduler::start () {
    worker_ = new Worker(&queue_, &running_);
    for (int i = 0; i < num_workers_; i++) {
        worker_pool_.push_back(
            std::thread(&Worker::start, worker_)
        );
    }

    for (auto& i : worker_pool_) { i.join(); }
    worker_pool_.clear();
}

std::atomic<bool>* Scheduler::get_running() {
    return &running_;
}

Scheduler::~Scheduler() {
    if (worker_ != nullptr) {
        delete worker_;
    }
}

}