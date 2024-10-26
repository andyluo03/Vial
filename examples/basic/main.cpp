#include "core/engine.hh"
#include "core/task.hh"
#include "core/dispatcher.hh"

#include <chrono>
#include <iostream>
#include <thread>
#include <sstream>

template <typename T>
using Task = vial::Task<T>;

Task<int> foo (int n) {
    std::stringstream s; s << "Calling foo(" << n << ") on thread: " << std::this_thread::get_id() << std::endl; std::cerr << s.str();
    co_return n;
}

Task<int> bar (int n) {
    std::stringstream s; s << "Begin bar(" << n << ") on thread: " << std::this_thread::get_id() << std::endl; std::cerr << s.str();
    co_await foo(n);

    std::stringstream t; t << "Finishing bar(" << n << ") on thread: " << std::this_thread::get_id() << std::endl; std::cerr << t.str();
    co_return 1;
}

class BasicDispatcher : public vial::Dispatcher {
  public:
    BasicDispatcher () = default;

    virtual vial::TaskBase* dispatch () override {
        std::stringstream s; s << "Dispatching from thread: " << std::this_thread::get_id() << std::endl; std::cerr << s.str();

        std::this_thread::sleep_for(std::chrono::seconds(1));
        return new Task<int>(bar( rand() % 2048 ));
    }

};

int main () {
    vial::Dispatcher* io = new BasicDispatcher();
    vial::Engine a(io);

    a.start();
}