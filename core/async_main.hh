#include "task.hh"
#include "engine.hh"
#include <iostream>

namespace vial {
    Scheduler Vial{};
}

extern vial::Task<int> async_main();

vial::Task<int> main_wrapper (std::atomic<bool>* running) {
    assert(co_await async_main());
    running->store(false);
    co_return 1;
}

int main () {
    auto entry = vial::Task<int>( main_wrapper(vial::Vial.get_running()) );
    vial::Vial.fire_and_forget( entry );
    vial::Vial.start();
    return 1;
}