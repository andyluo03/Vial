#include "engine.hh"
#include "worker.hh"

namespace vial {

Engine::Engine (size_t num_workers, size_t num_dispatchers)
    : num_dispatchers_{num_dispatchers}, num_workers_{num_workers} {}
    
void Engine::start () {
    for (size_t i = 0; i < num_workers_; i++) {
        auto worker = new Worker(&queue_);
        worker_pool_.push_back(
            std::thread(&Worker::start, worker)
        );
    }

    for (auto& i : worker_pool_) { i.join(); }
}

Engine::~Engine() = default;

void Engine::fire_and_forget(TaskBase* x) {
    queue_.enqueue(x);
}

template<typename T>
Fiber<T>::Fiber(Fiber&& other) noexcept 
    : shared_state_(std::move(other.shared_state_)),
      task_(std::exchange(other.task_, nullptr)) {}

template<typename T>
Fiber<T>& Fiber<T>::operator=(Fiber&& other) noexcept {
    if (this != &other) {
        delete task_;
        task_ = std::exchange(other.task_, nullptr);
        shared_state_ = std::move(other.shared_state_);
    }
    return *this;
}

template<typename T>
bool Fiber<T>::is_complete() const {
    std::lock_guard<std::mutex> lock(shared_state_->lock_);
    return shared_state_->is_done_;
}

template<typename T>
T Fiber<T>::get_result() {
    std::unique_lock<std::mutex> lock(shared_state_->lock_);
    shared_state_->complete_.wait(lock, [this] { 
        return shared_state_->is_done_; 
    });
    return shared_state_->result_;
}

template<typename T>
void Fiber<T>::notify_complete() {
    std::lock_guard<std::mutex> lock(shared_state_->lock_);
    shared_state_->result_ = task_->await_resume();
    shared_state_->is_done_ = true;
    shared_state_->complete_.notify_one();
}

template<typename T>
Fiber<T>::~Fiber() {
    delete task_;
}

template<>
void Fiber<void>::get_result() {
    std::unique_lock<std::mutex> lock(shared_state_->lock_);
    shared_state_->complete_.wait(lock, [this] { 
        return shared_state_->is_done_; 
    });
    task_->await_resume();
}

template<>
void Fiber<void>::notify_complete() {
    std::lock_guard<std::mutex> lock(shared_state_->lock_);
    task_->await_resume();
    shared_state_->is_done_ = true;
    shared_state_->complete_.notify_one();
}

template class Fiber<int>;
template class Fiber<void>;

Scheduler::Scheduler() = default;

Scheduler& Scheduler::instance() {
    static Scheduler scheduler;
    return scheduler;
}

template<typename T>
void Scheduler::spawn(Fiber<T>&& fiber) {
    if (fiber.task_) {
        auto fiber_ptr = std::make_shared<Fiber<T>>(std::move(fiber));
        fiber_ptr->task_->set_completion_callback([fiber_ptr]() {
            fiber_ptr->notify_complete();
        });
        queue_.enqueue(fiber_ptr->task_);
        fiber_ptr->task_ = nullptr;
    }
}

template void Scheduler::spawn<int>(Fiber<int>&&);
template void Scheduler::spawn<void>(Fiber<void>&&);

void Scheduler::run() {
    running_ = true;
    while (running_) {
        auto* task = queue_.get();
        if (!task) {
            if (running_) continue;
            break;
        }
        
        if (!running_) {
            delete task;
            break;
        }

        TaskState state = task->run();
        if (state == TaskState::kAwaiting && running_) {
            queue_.enqueue(task);
        } else if (state == TaskState::kComplete) {
            task->notify_complete();
        } else {
            delete task;
        }
    }
}

void Scheduler::shutdown() {
    running_ = false;
    queue_.enqueue(nullptr);
}

Scheduler::~Scheduler() = default;

}