#pragma once

#include <coroutine>
#include <atomic>

namespace vial {

enum TaskState {
  kAwaiting,
  kComplete
};

// DO NOT COPY/MOVE THIS.
class TaskBase {
  public:
    TaskBase() = default;

    // Use a promise type for moving around TaskWrappers. 
    TaskBase(TaskBase&) = delete;
    TaskBase(TaskBase&&) = delete;
    TaskBase& operator=(TaskBase&) = delete;
    TaskBase& operator=(TaskBase&&) = delete;

    virtual TaskState run () = 0;
    virtual TaskBase* awaiting () = 0;
    virtual TaskBase* callback () = 0;
    virtual ~TaskBase() {}
};

template <typename T>
class Task : public TaskBase {
  public:
    TaskState run() override {
      handle_.resume();
      return { handle_.promise().state_ };
    }

    virtual TaskBase* awaiting () override {
      return handle_.promise().awaiting_;
    }

    virtual TaskBase* callback () override {
      return handle_.promise().callback_;
    }

    struct promise_type {
      public:
        using Handle = std::coroutine_handle<promise_type>;
        
        Task<T> get_return_object() { return Task{Handle::from_promise(*this)}; }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; } 

        void return_value (T a) {
            result_ = a;
            state_ = kComplete;
        }

        void unhandled_exception() {}
        
        private:
          TaskState state_;
          T result_;
          TaskBase* awaiting_;
          TaskBase* callback_;

          friend Task<T>;
    };

    bool await_ready () const noexcept { 
      return handle_.promise().state_ == TaskState::kComplete;
    }

    template <typename S>
    void await_suspend(std::coroutine_handle<S> awaitee) noexcept {
      awaitee.promise().awaiting_ = this;
      handle_.promise().callback_ = new Task<T>{awaitee};
    } 

    T await_resume() const noexcept {
      return handle_.promise().result_;
    }

    explicit Task(const typename promise_type::Handle coroutine) : handle_{coroutine}, counter_{ new std::atomic<int>(1) } {}

    Task (Task& other) : counter_{other.counter_}, handle_{other.handle_} {
      counter_++;
    }

    Task& operator=(Task& other) {
      counter_ = other.counter_;
      handle_ = other.handle_;
      counter_++;

      return *this;
    }

    virtual ~Task() override {
      counter_--;
      if (counter_ == 0) {
        handle_.destroy();
      }
    }

    promise_type::Handle handle_;
    std::atomic<int>* counter_;
};

template<>
class Task<void> : public TaskBase {
  virtual TaskState run () override {
    return TaskState::kComplete;
  }
};


} // namespace vial