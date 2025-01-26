#pragma once

#include <coroutine>
#include <atomic>
#include <iostream>
#include <type_traits>

namespace vial {

enum TaskState : std::uint8_t {
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
    auto operator=(TaskBase&) -> TaskBase& = delete;
    auto operator=(TaskBase&&) -> TaskBase& = delete;

    //! Start/resume execution of the underlying coroutine.
    virtual auto run () -> TaskState = 0;

    //! Get a pointer to the awaiting coroutine.
    [[nodiscard]] virtual auto get_awaiting () const -> TaskBase* = 0;

    //! Get an pointer to a clone of TaskBase (points to the same underlying coroutine)
    [[nodiscard]] virtual auto clone() const -> TaskBase* = 0;

    //! 
    [[nodiscard]] virtual auto is_enqueued () const -> bool = 0;
    virtual void set_enqueued_true () = 0;
    virtual void set_enqueued_false () = 0;

    [[nodiscard]] virtual auto get_state () const -> TaskState = 0;

    virtual void set_callback(TaskBase*) = 0;
    [[nodiscard]] virtual auto get_callback() const -> TaskBase* = 0;

    //! Destroys the underlying coroutine (this should happen on co_return).
    virtual void destroy() = 0;
    virtual void print_promise_addr() = 0;

    virtual ~TaskBase() = default;
};

//! Task<T> wraps a std::coroutine_handle to provide callback logic. 
template <typename T> requires (std::is_copy_constructible_v<T>)
class Task : public TaskBase {
  public:
      //! Underlying heap allocated state of a coroutine.
    struct promise_type {
      public:
        using Handle = std::coroutine_handle<promise_type>;
        
        auto get_return_object() -> Task<T> { return Task{Handle::from_promise(*this)}; }

        //!
        auto initial_suspend() -> std::suspend_always { return {}; }

        //! Returning suspend_always means we must manually handle lifetimes
        auto final_suspend() noexcept -> std::suspend_always { return {}; } 

        //! On `co_return x` set state. 
        void return_value (T x) {
            result_ = T(x);
            state_ = kComplete;
        }

        //! Handler for unhandled exceptions. 
        void unhandled_exception() {}

        auto get_awaiting() -> TaskBase*& { return awaiting_; }
        
        private:
          TaskState state_ = TaskState::kAwaiting;

          // Ownership of task currently awaiting. (Delete on resumption). 
          TaskBase* awaiting_ = nullptr;

          // To be added back to queue on completion.
          TaskBase* callback_ = nullptr; 
          
          std::atomic<bool> enqueued_ = false;

          T result_ = T();
        friend Task<T>;
    };

    //! Is the (outside) task ready?
    [[nodiscard]] auto await_ready () const noexcept -> bool {
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
      
      try {
        awaitee_awaiting = new Task<T>{*this};
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
      }
    }

    //! Handler for returning a value 
    /*!
    Code Example:
      Task<T> foo ();
      ...

      co_await foo(); // the value here is the return value of await_resume();
    */
    auto await_resume() noexcept -> T {
      auto& awaiting = this->handle_.promise().awaiting_;
      delete awaiting;
      awaiting = nullptr;
      return handle_.promise().result_;
    }

    //! Construct a Task from a coroutine handle.
    explicit Task(const typename promise_type::Handle coroutine) : handle_{coroutine} {}

    //! Copy constructor.
    Task (const Task& other) : handle_{other.handle_} {}

    //! Move constructor.
    Task (const Task&& other)  noexcept : handle_{other.handle_} {} 

    //! Copy assignment operator.
    auto operator=(const Task& other) -> Task& {
      handle_ = other.handle_;
      return *this;
    }

    //! Move assignment operator.
    auto operator=(Task&& other) noexcept -> Task& {
      handle_ = other.handle_;
      return *this;
    }

    ~Task() override = default;

    //! Virtual clone. 
     [[nodiscard]] auto clone () const -> TaskBase* override {
      return new Task<T>(*this);
    }

     auto run() -> TaskState override {
      handle_.resume();
      return { handle_.promise().state_ };
    }

    [[nodiscard]] auto get_awaiting () const -> TaskBase* override {
      return handle_.promise().awaiting_;
    }

    [[nodiscard]] auto is_enqueued () const -> bool override {
      return this->handle_.promise().enqueued_.load(std::memory_order_release);
    }

     void set_enqueued_true () override {
      this->handle_.promise().enqueued_.store(true, std::memory_order_acquire);
    }

     void set_enqueued_false () override {
      this->handle_.promise().enqueued_.store(false, std::memory_order_acquire);
    }

    //!
    [[nodiscard]] auto get_state () const -> TaskState override {
      return this->handle_.promise().state_;
    }

    //!
    void set_callback (TaskBase* x) override {
      this->handle_.promise().callback_ = x;
    }

    //! 
    [[nodiscard]] auto get_callback () const -> TaskBase* override {
      return this->handle_.promise().callback_;
    }

    //!
    void print_promise_addr() override {
      std::cout << handle_.address() << std::endl;
    }

    //! 
    void destroy () override {
      handle_.destroy();
    }

  private:
    promise_type::Handle handle_;
};

} // namespace vial