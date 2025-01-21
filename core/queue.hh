#pragma once 

#include <mutex>
#include <iostream>
#include <optional>

namespace vial {

constexpr size_t kMaxQueueSize = 4096;

template <typename T>
class Queue {
  public:
    Queue() = default;

    void enqueue(T);
    std::optional<T> get();

  private:
    T contents_[kMaxQueueSize];
    std::mutex lock_;

    size_t read_ptr_ = 0;
    size_t write_ptr_ = 0;
    size_t size_ = 0;
};

template <typename T>
void Queue<T>::enqueue (T a) {
    std::lock_guard<std::mutex> lock(lock_);

    contents_[write_ptr_] = a;
    write_ptr_ = (write_ptr_ + 1) % kMaxQueueSize;
    size_++;
}

template <typename T>
std::optional<T> Queue<T>::get() {
    if ( lock_.try_lock() ) {

        if (size_ > 0) {
            auto res = contents_[read_ptr_];

            read_ptr_ = (read_ptr_ + 1) % kMaxQueueSize;
            size_--;
            
            lock_.unlock();
            return res;
        }

        lock_.unlock();
    }
    
    return std::nullopt;
}

}