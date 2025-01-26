#pragma once 

#include <mutex>
#include <cassert>
#include <optional>
#include <queue>

namespace vial {

constexpr size_t kMaxQueueSize = 16384;

template <typename T>
class Queue {
  public:
    Queue() = default;

    void enqueue(T item);
    auto try_get() -> std::optional<T>;

  private:
    std::mutex lock_;
    std::queue<T> contents_;
};

template <typename T>
void Queue<T>::enqueue (T item) {
    std::lock_guard<std::mutex> lock(lock_);
    contents_.push(item);
}

template <typename T>
auto Queue<T>::try_get() -> std::optional<T> {
    std::lock_guard<std::mutex> lock(lock_);

    if (!contents_.empty()) {
        T res = contents_.front();
        contents_.pop();
        return res;
    }

    return std::nullopt;
}

}