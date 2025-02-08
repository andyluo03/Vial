#include "task.hh"
#include "scheduler.hh"

#include <cassert>

namespace vial {
    Scheduler Vial{}; //NOLINT
}

extern auto async_main() -> vial::Task<int>;

auto main_wrapper () -> vial::Task<int> {
    assert(co_await async_main());
    vial::Vial.stop();
    co_return 1;
}

auto main () -> int {
    vial::Vial.fire_and_forget( main_wrapper() );
    vial::Vial.start();
    return 1;
}