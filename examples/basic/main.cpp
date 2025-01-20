#include "core/engine.hh"
#include "core/task.hh"
#include "core/async_main.hh"
#include <chrono>
#include <iostream>
#include <thread>
#include <sstream>

vial::Task<int> bar(int a) {
    std::cout << "Bar: " << a << " on thread: " << std::this_thread::get_id() << std::endl;
    co_return a + 5;
}

vial::Task<int> foo(int a) {
    std::cout << "Foo: " << a << " on thread: " << std::this_thread::get_id() << std::endl;
    co_return co_await bar(a) + co_await bar(a + 7);
}

vial::Task<int> async_main() {
    int result = co_await foo(7);

    std::cout << result << std::endl;
    
    co_return 1;
}