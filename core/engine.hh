#pragma once 

#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "queue.hh"
#include "worker.hh"
#include "task.hh"

namespace vial {

class Engine {
  public:
    Engine (
      size_t num_workers = std::max(static_cast<int>(std::thread::hardware_concurrency()) - 2, 2), 
      size_t num_dispatchers = 1
    );

    void start();

    void fire_and_forget(TaskBase*);

    ~Engine();

  private:
    Queue<TaskBase*> queue_;
    Worker* worker_;

    std::vector<std::thread> dispatcher_pool_;
    std::vector<std::thread> worker_pool_;

    size_t num_dispatchers_;
    size_t num_workers_;
};

template<typename T>
class Fiber {
public:
    Fiber() = delete;
    Fiber(const Fiber&) = delete;
    Fiber& operator=(const Fiber&) = delete;
    
    Fiber(Fiber&& other) noexcept;
    Fiber& operator=(Fiber&& other) noexcept;

    template<typename Func>
    static Fiber<T> create(Func func) {
        auto fiber = Fiber<T>{new Task<T>(func())};
        fiber.shared_state_ = std::make_shared<SharedState>();
        return fiber;
    }
    
    bool is_complete() const;
    T get_result();
    void notify_complete();
    ~Fiber();

    friend class Scheduler;

    struct SharedState {
        mutable std::mutex lock_;
        std::condition_variable complete_;
        bool is_done_{false};
        T result_;
    };

    std::shared_ptr<SharedState> get_shared_state() const { 
        return shared_state_; 
    }

private:
    Task<T>* task_;
    std::shared_ptr<SharedState> shared_state_;
    explicit Fiber(Task<T>* task) : task_(task), shared_state_() {}
};

template<>
struct Fiber<void>::SharedState {
    mutable std::mutex lock_;
    std::condition_variable complete_;
    bool is_done_{false};
};

template<>
void Fiber<void>::notify_complete();

class Scheduler {
  public:
    // simple singleton
    static Scheduler& instance();

    template<typename T>
    void spawn(Fiber<T>&& fiber);
    void run();
    void shutdown();
    void reset() {
        shutdown();
        running_ = false;
        while (auto* task = queue_.try_get()) {
            delete task;
        }
    }

  private:
    Queue<TaskBase*> queue_;
    std::atomic<bool> running_{false};

    Scheduler();
    ~Scheduler();
};

}