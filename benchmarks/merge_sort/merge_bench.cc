#include <algorithm>
#include <benchmark/benchmark.h>
#include <vector>

#include "core/task.hh"
#include "core/engine.hh"
#include <algorithm>

constexpr size_t kSmallBenchSize = 15;

// sorts input from [left, right)
vial::Task<int> merge_sort (vial::Engine* engine, std::vector<int>* input, int left, int right) {
    if (right - left <= 1) {
        co_return 1;
    }

    if (right - left == 2) {
        if ( (*input)[left] > (*input)[left+1]) {
            std::swap((*input)[left], (*input)[left+1]);
        }

        co_return 1;
    }

    int mid = (left + right) / 2;

    auto sort_left = engine->spawn_task( merge_sort(engine, input, left, mid) );
    auto sort_right = engine->spawn_task( merge_sort(engine, input, mid, right) );

    assert(co_await sort_left);
    assert(co_await sort_right);

    std::vector<int> merged_result;

    int l_b = left;
    int r_b = mid;

    while (l_b < mid && r_b < right) {
        if ( (*input) [l_b] < (*input) [r_b]) {
            merged_result.push_back((*input)[l_b]);
            l_b++;
        } else {
            merged_result.push_back((*input)[r_b]);
            r_b++;
        }
    }

    while (l_b < mid) {
        merged_result.push_back((*input)[l_b]);
        l_b++;
    }

    while (r_b < right) {
        merged_result.push_back((*input)[r_b]);
        r_b++;
    }

    for (int i = left; i < right; i++) {
        (*input)[i] = merged_result[i - left];
    }

    co_return 1;
}

vial::Task<int> wrapped_main (vial::Engine* engine, std::atomic<bool>* stopper, std::vector<int>* a) {
    co_await merge_sort(engine, a, 0, a->size());
    *stopper = false;
    co_return 1;
}

static void BM_parallel_small(benchmark::State& s) {
    vial::Engine scheduler;

    for (auto _ : s) {
        std::vector<int> a;
        for (int i = 0; i < kSmallBenchSize; i++) {
            a.push_back(i % 10);
        }

        scheduler.fire_and_forget( wrapped_main(&scheduler, scheduler.get_running(), &a)  );
        scheduler.start();

        for (int i = 0; i < a.size(); i++) {
            benchmark::DoNotOptimize(a[i]);
        }
    }
}

static void BM_std_small(benchmark::State& s) {
    for (auto _ : s) {
        std::vector<int> a;

        for (int i = 0; i < kSmallBenchSize; i++) {
            a.push_back(i % 10);
        }

        sort(a.begin(), a.end());

        for (int i = 0; i < a.size() - 1; i++) {
            benchmark::DoNotOptimize(a[i]);
        }
    }
}


BENCHMARK(BM_parallel_small)->Iterations(10);
BENCHMARK(BM_std_small)->Iterations(10);

BENCHMARK_MAIN();