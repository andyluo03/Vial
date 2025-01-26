#include "core/task.hh"
#include "core/async_main.hh"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

constexpr int kDefaultReturn = 10;
constexpr size_t kNumPromises = 10;

auto foo() -> vial::Task<int> {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    co_return kDefaultReturn;
}

auto bar() -> vial::Task<int> {
    std::vector<vial::Task<int>> promises;
    promises.reserve(kNumPromises);

    for (int i = 0; i < kNumPromises; i++) {
        promises.push_back( vial::Vial.spawn_task(foo()) ); 
    }
    
    std::atomic<int> sum = 0;

    for (int i = 0; i < kNumPromises; i++) {
        sum += co_await promises[i];
    }

    co_return sum.load();
}

auto async_main() -> vial::Task<int> {
    auto result = vial::Vial.spawn_task(bar());

    std::cout << (co_await result) << std::endl;
    co_return 1;
}
