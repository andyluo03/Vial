#pragma once

#include <coroutine>
#include <vector>
#include <iostream>

namespace vial {

enum TaskState {
  kAwaiting,
  kComplete,
  kStop
};

struct kStopper {};

//! TaskBase is a type-erased base class for Task<T> used for callbacks. 
class TaskBase {
  public:
    TaskBase() = default;

    // Use clone instead. 
    TaskBase(TaskBase&) = delete;
    TaskBase(TaskBase&&) = delete;
    TaskBase& operator=(TaskBase&) = delete;
    TaskBase& operator=(TaskBase&&) = delete;

    //! Start/resume execution of the underlying coroutine.
    virtual TaskState run () = 0;

    //! Get a pointer to the coroutine being awaited upon. 
    virtual TaskBase* awaiting () = 0;

    //! Get pointers to callbacks after task completion.
    virtual std::vector<TaskBase*> callbacks() = 0;

    //! Get an pointer to a clone of TaskBase (points to the same underlying coroutine)
    virtual TaskBase* clone() = 0;

    //! Returns whether or not the current task has been queued.
    virtual bool enqueued () = 0;

    //! 
    virtual void set_enqueued () = 0;

    //!
    virtual TaskState get_state () = 0;

    //! Destroys the underlying coroutine (this should happen on co_return).
    virtual void destroy() = 0;

    //! 
    virtual void register_callback(TaskBase*) = 0;

    //!
    virtual void print_promise_addr() = 0;

    virtual ~TaskBase() {}
};

//! Task<T> wraps a std::coroutine_handle to provide callback logic. 
template <typename T>
class Task : public TaskBase {
  public:
    virtual TaskState run() override {
      handle_.resume();
      return { handle_.promise().state_ };
    }

    virtual TaskBase* awaiting () override {
      return handle_.promise().awaiting_;
    }

    virtual std::vector<TaskBase*> callbacks () override {
      return handle_.promise().callbacks_;
    }

    //! Underlying heap allocated state of a coroutine.
    struct promise_type {
      public:
        using Handle = std::coroutine_handle<promise_type>;
        
        Task<T> get_return_object() { return Task{Handle::from_promise(*this)}; }

        //!
        std::suspend_always initial_suspend() { return {}; }

        //! Returning suspend_always means we must manually handle lifetimes
        std::suspend_always final_suspend() noexcept { return {}; } 

        //! On `co_return x` what set state. 
        void return_value (T x) {
            result_ = x;
            state_ = kComplete;
        }

        //! Handler for unhandled exceptions. 
        void unhandled_exception() {}
        
        private:
          TaskState state_;
          
          TaskBase* awaiting_ = nullptr;
          std::vector<TaskBase*> callbacks_;
          
          bool enqueued_ = false;

          T result_;
        friend Task<T>;
    };

    //! Is the (outside) task ready?
    bool await_ready () const noexcept {
      return false;
    }

    //! Handler for co_await where `this` is the task begin awaited upon. 
    /*! Suspicious...
    */
    template <typename S>
    void await_suspend(std::coroutine_handle<S> awaitee) noexcept {
      // Propogated to workers to enqueue the awaited upon task. 
      awaitee.promise().awaiting_ = new Task<T>{*this};
    }

    //! Handler for returning a value 
    /*!
    Code Example:
      Task<T> foo ();
      ...

      co_await foo(); // the value here is the return value of await_resume();
    */
    T await_resume() const noexcept {
      return handle_.promise().result_;
    }

    //! Construct a Task from a coroutine handle.
    explicit Task(const typename promise_type::Handle coroutine) : handle_{coroutine} {}

    //! Copy constructor.
    Task (const Task& other) : handle_{other.handle_} {}

    //! Copy assignment operator.
    Task& operator=(const Task& other) {
      handle_ = other.handle_;
      return *this;
    }

    //! Virtual clone. 
    virtual TaskBase* clone () override {
      return new Task<T>(*this);
    }

    virtual bool enqueued () override {
      return this->handle_.promise().enqueued_;
    }

    virtual void set_enqueued () override {
      this->handle_.promise().enqueued_ = true;
    }

        //!
    virtual TaskState get_state () override {
      return this->handle_.promise().state_;
    }

    //!
    virtual void register_callback (TaskBase* x) override {
      this->handle_.promise().callbacks_.push_back(x->clone());
    }

    //! 
    virtual void destroy () override {
      handle_.destroy();
    }

     //!
    virtual void print_promise_addr() override {
      std::cout << handle_.address() << std::endl;
    }

  //private:
    promise_type::Handle handle_;
};

template<>
class Task<void> : public TaskBase {
  virtual TaskState run () override {
    return TaskState::kComplete;
  }
};


} // namespace vial