#pragma once 

#include <mutex>
#include <iostream>
#include <cassert>
#include <optional>
#include <queue>

namespace vial {

constexpr size_t kMaxQueueSize = 16384;

template <typename T>
class Queue {
  public:
    Queue() = default;

    void enqueue(T);
    std::optional<T> try_get();

  private:
    std::mutex lock_;
    std::queue<T> contents_;
};

template <typename T>
void Queue<T>::enqueue (T a) {
    std::lock_guard<std::mutex> lock(lock_);
    contents_.push(a);
}

template <typename T>
std::optional<T> Queue<T>::try_get() {
    std::lock_guard<std::mutex> lock(lock_);

    if (contents_.size() > 0) {
        T res = contents_.front();
        contents_.pop();
        return res;
    }

    return std::nullopt;
}

}