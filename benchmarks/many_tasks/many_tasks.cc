#include <benchmark/benchmark.h>
#include <cstdlib>
#include <vector>
#include <cstdlib>

#include "core/scheduler.hh"

constexpr int kNumTasksSmall  = 5;
constexpr int kNumTasksMedium = 3e3;
constexpr int kNumTasksLarge = 1e7;

vial::Task<int> foo () {
    // Randomness removes compiler optimizing away things!
    co_return rand() % 2;
}

vial::Task<int> bar (vial::Scheduler* engine, int num_tasks, std::atomic<int>* result) {
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
    vial::Scheduler scheduler;
    srand(time(NULL));

    for (auto _ : s) {
        scheduler.fire_and_forget(bar(&scheduler, kNumTasksSmall, &result));
        scheduler.start();
        benchmark::DoNotOptimize( result.load() );
    }
}

static void BM_medium(benchmark::State& s) {
    std::atomic<int> result{0};

    vial::Scheduler scheduler;
    for (auto _ : s) {
        scheduler.fire_and_forget(bar(&scheduler, kNumTasksMedium, &result));
        scheduler.start();
        benchmark::DoNotOptimize( result.load() );
    }
}

static void BM_large(benchmark::State& s) {
    std::atomic<int> result{0};

    vial::Scheduler scheduler;
    for (auto _ : s) {
        scheduler.fire_and_forget(bar(&scheduler, kNumTasksLarge, &result));
        scheduler.start();
        benchmark::DoNotOptimize( result.load() );
    }
}

BENCHMARK(BM_small);
BENCHMARK(BM_medium);
BENCHMARK(BM_large);

BENCHMARK_MAIN();
