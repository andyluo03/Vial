#pragma once 

#include <unordered_map>
#include <mutex>

namespace vial {

constexpr size_t kMaxQueueSize = 4096;

template <typename T>
class Queue {
  public:
    Queue() = default;

    void enqueue(T);
    T get();

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
T Queue<T>::get() {
    while (true) {
        std::lock_guard<std::mutex> lock(lock_);

        if (size_ > 0) {
            auto res = contents_[read_ptr_];

            read_ptr_ = (read_ptr_ + 1) % kMaxQueueSize;
            size_--;

            return res;
        }
    }
}

}