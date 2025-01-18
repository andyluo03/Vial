#include "task.hh"
#include "engine.hh"
#include <iostream>
vial::Engine engine;

extern vial::Task<int> async_main();

int main () {
    auto entry = new vial::Task<int>(async_main());
    engine.fire_and_forget( entry );
    engine.start();
    delete entry;
    return 1;
}