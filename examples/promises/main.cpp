#include "core/task.hh"
#include "core/async_main.hh"

#include <iostream>
#include <thread>
#include <coroutine>
#include <vector>

vial::Task<int> foo() {
    sleep(1);

    co_return 10;
}

vial::Task<int> bar() {
    std::vector<vial::Task<int>> promises;
    for (int i = 0; i < 10; i++) {
        promises.push_back( vial::Vial.spawn_task(foo()) ); 
    }
    
    std::atomic<int> sum = 0;

    for (int i = 0; i < 10; i++) {
        sum += co_await promises[i];
    }

    co_return sum.load();
}

vial::Task<int> async_main() {
    auto result = vial::Vial.spawn_task(bar());

    std::cout << (co_await result) << std::endl;

    co_return 1;
}
