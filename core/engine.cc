#include "engine.hh"
#include "dispatcher.hh"
#include "worker.hh"

namespace vial {

Engine::Engine (Dispatcher* dispatcher, size_t num_workers, size_t num_dispatchers)
    : dispatcher_{dispatcher}, num_workers_{num_workers}, num_dispatchers_{num_dispatchers} {}
    
void Engine::start () {
    worker_ = new Worker(&queue_);
    for (int i = 0; i < num_workers_; i++) {
        worker_pool_.push_back(
            std::thread(&Worker::start, worker_)
        );
    }

    for (int i = 0; i < num_dispatchers_; i++) {
        dispatcher_pool_.push_back(std::thread([this]() {
            while (true) {
                queue_.enqueue(dispatcher_->dispatch());
            }
        }));
    }

    for (auto& i : dispatcher_pool_) { i.join(); }
    for (auto& i : worker_pool_) { i.join(); }
}

Engine::~Engine() = default;

}