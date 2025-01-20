#pragma once 

#include <mutex>
#include <iostream>
#include <condition_variable>

namespace vial {

constexpr size_t kMaxQueueSize = 4096;

template <typename T>
class Queue {
  public:
    Queue() = default;

    void enqueue(T);
    T get();
    T try_get() {
        std::lock_guard<std::mutex> lock(lock_);
        if (size_ == 0) return nullptr;
        auto res = contents_[read_ptr_];
        read_ptr_ = (read_ptr_ + 1) % kMaxQueueSize;
        size_--;
        return res;
    }

  private:
    T contents_[kMaxQueueSize];
    std::mutex lock_;
    std::condition_variable not_empty_;

    size_t read_ptr_ = 0;
    size_t write_ptr_ = 0;
    size_t size_ = 0;
};

template <typename T>
void Queue<T>::enqueue (T a) {
    {
        std::lock_guard<std::mutex> lock(lock_);
        contents_[write_ptr_] = a;
        write_ptr_ = (write_ptr_ + 1) % kMaxQueueSize;
        size_++;
    }
    not_empty_.notify_one();
}

template <typename T>
T Queue<T>::get() {
    std::unique_lock<std::mutex> lock(lock_);
    not_empty_.wait(lock, [this] { return size_ > 0; });
    
    auto res = contents_[read_ptr_];
    read_ptr_ = (read_ptr_ + 1) % kMaxQueueSize;
    size_--;
    return res;
}

}