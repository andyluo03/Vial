#include "task.hh"
#include "scheduler.hh"

namespace vial {
    Scheduler Vial{}; //NOLINT
}

extern auto async_main() -> vial::Task<int>;

auto main_wrapper (std::atomic<bool>* running) -> vial::Task<int> {
    assert(co_await async_main());
    running->store(false);
    co_return 1;
}

auto main () -> int {
    auto entry = vial::Task<int>( main_wrapper(vial::Vial.get_running()) );
    vial::Vial.fire_and_forget( entry );
    vial::Vial.start();
    return 1;
}