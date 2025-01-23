#pragma once

#include <coroutine>
#include <atomic>
#include <iostream>
#include <type_traits>

namespace vial {

enum TaskState {
  kAwaiting,
  kComplete
};

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

    //! Get a pointer to the awaiting coroutine.
    virtual TaskBase* get_awaiting () const = 0;

    //! Get an pointer to a clone of TaskBase (points to the same underlying coroutine)
    virtual TaskBase* clone() = 0;

    //! 
    virtual bool is_enqueued () = 0;
    virtual void set_enqueued_true () = 0;
    virtual void set_enqueued_false () = 0;

    virtual TaskState get_state () const = 0;

    virtual void set_callback(TaskBase*) = 0;
    virtual TaskBase* get_callback() const = 0;

    //! Destroys the underlying coroutine (this should happen on co_return).
    virtual void destroy() = 0;
    virtual void print_promise_addr() = 0;

    virtual ~TaskBase() {}
};

//! Task<T> wraps a std::coroutine_handle to provide callback logic. 
template <typename T> requires (std::is_copy_constructible<T>::value)
class Task : public TaskBase {
  public:
      //! Underlying heap allocated state of a coroutine.
    struct promise_type {
      public:
        using Handle = std::coroutine_handle<promise_type>;
        
        Task<T> get_return_object() { return Task{Handle::from_promise(*this)}; }

        //!
        std::suspend_always initial_suspend() { return {}; }

        //! Returning suspend_always means we must manually handle lifetimes
        std::suspend_always final_suspend() noexcept { return {}; } 

        //! On `co_return x` set state. 
        void return_value (T x) {
            result_ = x;
            state_ = kComplete;
        }

        //! Handler for unhandled exceptions. 
        void unhandled_exception() {}

        TaskBase*& get_awaiting() { return awaiting_; }
        
        private:
          TaskState state_ = TaskState::kAwaiting;

          // Ownership of task currently awaiting. (Delete on resumption). 
          TaskBase* awaiting_ = nullptr;

          // To be added back to queue on completion.
          TaskBase* callback_ = nullptr; 
          
          std::atomic<bool> enqueued_ = false;

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
      auto& awaitee_awaiting = awaitee.promise().get_awaiting();

      if (awaitee_awaiting != nullptr) {
        delete awaitee_awaiting;
        awaitee_awaiting = nullptr;
      }
      
      awaitee_awaiting = new Task<T>{*this};
    }

    //! Handler for returning a value 
    /*!
    Code Example:
      Task<T> foo ();
      ...

      co_await foo(); // the value here is the return value of await_resume();
    */
    T await_resume() const noexcept {
      auto& awaiting = this->handle_.promise().awaiting_;
      delete awaiting;
      awaiting = nullptr;

      T res = handle_.promise().result_;
      return res;
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

    virtual TaskState run() override {
      handle_.resume();
      return { handle_.promise().state_ };
    }

    virtual TaskBase* get_awaiting () const override {
      return handle_.promise().awaiting_;
    }

    virtual bool is_enqueued () override {
      return this->handle_.promise().enqueued_.load(std::memory_order_release);
    }

    virtual void set_enqueued_true () override {
      this->handle_.promise().enqueued_.store(true, std::memory_order_acquire);
    }

    virtual void set_enqueued_false () override {
      this->handle_.promise().enqueued_.store(false, std::memory_order_acquire);
    }

    //!
    virtual TaskState get_state () const override {
      return this->handle_.promise().state_;
    }

    //!
    virtual void set_callback (TaskBase* x) override {
      this->handle_.promise().callback_ = x;
    }

    //! 
    virtual TaskBase* get_callback () const override {
      return this->handle_.promise().callback_;
    }

    //!
    virtual void print_promise_addr() override {
      std::cout << handle_.address() << std::endl;
    }

    //! 
    virtual void destroy () override {
      handle_.destroy();
    }

  private:
    promise_type::Handle handle_;
};

} // namespace vial