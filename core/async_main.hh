#include "task.hh"
#include "engine.hh"
#include <iostream>

namespace vial {
    Engine Vial{};
}

extern vial::Task<int> async_main();

vial::Task<int> main_wrapper (std::atomic<bool>* running) {
    co_await async_main();
    running->store(false);
    co_return 1;
}

int main () {
    auto entry = vial::Task<int>( main_wrapper(vial::Vial.get_running()) );
    vial::Vial.fire_and_forget( entry );
    vial::Vial.start();
    //vial::Vial.shutdown();
    return 1;
}