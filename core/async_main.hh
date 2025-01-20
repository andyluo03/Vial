#include "task.hh"
#include "engine.hh"
#include <iostream>

namespace vial {
    Engine Vial{};
}

extern vial::Task<int> async_main();

int main () {
    auto entry = vial::Task<int>( async_main() );
    vial::Vial.fire_and_forget( entry );
    vial::Vial.start();
    //vial::Vial.shutdown();
    return 1;
}