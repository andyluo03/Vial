#include <benchmark/benchmark.h>
#include <map>
#include <mutex>

#include "core/scheduler.hh"
#include "core/task.hh"

constexpr size_t kWorkers = 8;
constexpr size_t kSmallTestSize = 128;
constexpr size_t kMediumTestSize = 1e6;
constexpr size_t kLargeTestSize = 1e8;

// (Simplified) Map-reduce counter!

vial::Task<int> mapper (std::vector<int>* input, int l, int r, std::map<int, int>* result) {
    for (size_t i = l; i < r; i++) {
        (*result)[ (*input)[i] ]++;
    }
    co_return 1;
}

vial::Task<int> shuffle (std::map<int, int>* input, std::vector<std::pair<int, int>> buckets[], std::mutex bucket_locks[]) {
    for (auto& p : *input) {
        auto bucket_id = p.first % kWorkers;
        std::lock_guard g(bucket_locks[bucket_id]);
        buckets[bucket_id].push_back(p);
    }

    co_return 1;
}

vial::Task<int> reduce ( std::map<int, int>* counts, std::vector<std::pair<int, int>>& pairs) {
    for (auto& p : pairs) {
        (*counts)[p.first] += p.second;
    }
    co_return 1;
}

vial::Task<int> counter ( std::map<int, int>* final_result, vial::Scheduler* scheduler, std::vector<int>* input ) {
    std::vector<vial::Task<std::map<int, int>>> tasks;

    // Map
    std::vector<vial::Task<int>> mappers;
    std::map<int, int> reduce_results[kWorkers];
    size_t step_size = (input->size() + kWorkers - 1) / kWorkers;

    for (size_t i = 0; i < kWorkers; i++) {
        mappers.push_back( 
            scheduler->spawn_task(
                mapper(input, i * step_size, std::min(input->size(), (i + 1) * step_size ), &reduce_results[i])
            ) 
        );
    }

    for (auto& i : mappers) co_await i;

    // Shuffle
    std::vector<std::pair<int, int>> buckets[kWorkers];
    std::mutex bucket_mutexes[kWorkers];
    std::vector<vial::Task<int>> shufflers;

    for (size_t i = 0; i < kWorkers; i++) {
        shufflers.push_back(
            scheduler->spawn_task( shuffle (&reduce_results[i], buckets, bucket_mutexes) )
        );
    }
    for (auto& i : shufflers) co_await i;
    
    // Reduce
    std::map<int, int> counts[kWorkers];
    std::vector<vial::Task<int>> reducers;

    for (size_t i = 0; i < kWorkers; i++) {
        reducers.push_back( 
            scheduler->spawn_task(
                reduce(&counts[i], buckets[i])
            )
        );
    }
    for (auto& i : reducers) co_await i;

    // Join Results (Unfortunately single-threaded! as std::map is not thread-safe). 
    for (size_t i = 0; i < kWorkers; i++) {
        for(const auto& j : counts[i]) {
            (*final_result)[j.first] = j.second;
        }
    }
    
    *(scheduler->get_running()) = false;
    co_return 1;
}

static void BM_parallel_small(benchmark::State& s) {
    vial::Scheduler scheduler;
    srand(time(NULL));
    std::vector<int> test;
    auto result = std::map<int, int>();

    for (int i = 0; i < kSmallTestSize; i++) {
        test.push_back(rand() % 256);
    }

    for (auto _ : s) {
        auto res = scheduler.spawn_task( counter(&result, &scheduler, &test) );
        scheduler.start();
    }
}

static void BM_standard_small(benchmark::State& s) {
    srand(time(NULL));
    std::vector<int> test;
    auto result = std::map<int, int>();

    for (int i = 0; i < kSmallTestSize; i++) {
        test.push_back(rand() % 256);
    }

    for (auto _ : s) {
        for (int i = 0; i < kSmallTestSize; i++) {
            result[ test[i] ]++;
        }
    }
}

static void BM_parallel_medium(benchmark::State& s) {
    vial::Scheduler scheduler;
    srand(time(NULL));
    std::vector<int> test;
    auto result = std::map<int, int>();

    for (int i = 0; i < kMediumTestSize; i++) {
        test.push_back(rand() % 256);
    }

    for (auto _ : s) {
        auto res = scheduler.spawn_task( counter(&result, &scheduler, &test) );
        scheduler.start();
    }
}

static void BM_standard_medium(benchmark::State& s) {
    srand(time(NULL));
    std::vector<int> test;
    auto result = std::map<int, int>();

    for (int i = 0; i < kMediumTestSize; i++) {
        test.push_back(rand() % 256);
    }

    for (auto _ : s) {
        for (int i = 0; i < kMediumTestSize; i++) {
            result[ test[i] ]++;
        }
    }
}


static void BM_parallel_large(benchmark::State& s) {
    vial::Scheduler scheduler;
    srand(time(NULL));
    std::vector<int> test;
    auto result = std::map<int, int>();

    for (int i = 0; i < kLargeTestSize; i++) {
        test.push_back(rand() % 256);
    }

    for (auto _ : s) {
        auto res = scheduler.spawn_task( counter(&result, &scheduler, &test) );
        scheduler.start();
    }
}

static void BM_standard_large(benchmark::State& s) {
    srand(time(NULL));
    std::vector<int> test;
    auto result = std::map<int, int>();

    for (int i = 0; i < kLargeTestSize; i++) {
        test.push_back(rand() % 256);
    }

    for (auto _ : s) {
        for (int i = 0; i < kLargeTestSize; i++) {
            result[ test[i] ]++;
        }
    }
}

BENCHMARK(BM_parallel_small);
BENCHMARK(BM_standard_small);
BENCHMARK(BM_parallel_medium);
BENCHMARK(BM_standard_medium);
BENCHMARK(BM_parallel_large);
BENCHMARK(BM_standard_large);
