#include "engine.hh"
#include "worker.hh"

namespace vial {

Engine::Engine (size_t num_workers, size_t num_dispatchers)
    : num_dispatchers_{num_dispatchers}, num_workers_{num_workers} {}
    
void Engine::start () {
    worker_ = new Worker(&queue_, &running_);
    for (int i = 0; i < num_workers_; i++) {
        worker_pool_.push_back(
            std::thread(&Worker::start, worker_)
        );
    }

    for (auto& i : worker_pool_) { i.join(); }
}

std::atomic<bool>* Engine::get_running() {
    return &running_;
}

Engine::~Engine() = default;

}