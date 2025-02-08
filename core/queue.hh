#pragma once

#include <queue>
#include <mutex>
#include <optional>

namespace vial {

template <typename T>
class Queue {
  public:
    Queue() = default;

    auto try_get () -> std::optional<T> {
      std::lock_guard guard(lock_);

      if (contents_.empty()) { return std::nullopt; }

      T res = contents_.front();
      contents_.pop();
      return res;
    }

    auto push (T item) {
      std::lock_guard guard(lock_);
      contents_.push(item);
    }

    [[nodiscard]] auto size () const -> size_t {
      std::lock_guard guard(lock_);
      return contents_.size();
    }

  private:
    std::mutex lock_;
    std::queue<T> contents_;
};

};