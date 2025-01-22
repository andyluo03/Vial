#include <benchmark/benchmark.h>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <cstdlib>

#include "core/engine.hh"

constexpr int kNumTasksSmall  = 5;
constexpr int kNumTasksMedium = 3000;

vial::Task<int> foo () {
    // Randomness removes compiler optimizing away things!
    co_return rand() % 2;
}

vial::Task<int> bar (vial::Engine* engine, int num_tasks, std::atomic<int>* result) {
    std::vector<vial::Task<int>> tasks;
    srand(time(NULL));

    for (int i = 0; i < num_tasks; i++) {
        tasks.push_back(engine->spawn_task(foo()));
    }

    int sum = 0;

    for (auto i : tasks) {
        sum += co_await i;
    }

    *engine->get_running() = false;

    *result = sum;
    co_return 1;
}

static void BM_small(benchmark::State& s) {
    std::atomic<int> result{0};
    vial::Engine scheduler;
    srand(time(NULL));


    for (auto _ : s) {
        scheduler.fire_and_forget(bar(&scheduler, kNumTasksSmall, &result));
        scheduler.start();
        benchmark::DoNotOptimize( result.load() );
    }
}

static void BM_medium(benchmark::State& s) {
    std::atomic<int> result{0};

    vial::Engine scheduler;
    for (auto _ : s) {
        scheduler.fire_and_forget(bar(&scheduler, kNumTasksMedium, &result));
        scheduler.start();
        benchmark::DoNotOptimize( result.load() );
    }
}

BENCHMARK(BM_small)->Iterations(50);
BENCHMARK(BM_medium)->Iterations(50);

BENCHMARK_MAIN();
