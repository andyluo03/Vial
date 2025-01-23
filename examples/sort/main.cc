#include "core/task.hh"
#include "core/async_main.hh"

#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>

constexpr long long kTestSize = 1e6;
constexpr long long kRuns = 100;

vial::Task<int> merge_sort (std::vector<int>* input, int left, int right) {
    if (right - left <= 1) {
        co_return 1;
    }

    if (right - left <= 256) {
        std::sort(input->begin() + left, input->begin() + right);
        co_return 1;
    }

    int mid = (left + right) / 2;

    auto sort_left = vial::Vial.spawn_task( merge_sort(input, left, mid) );
    auto sort_right = vial::Vial.spawn_task( merge_sort(input, mid, right) );

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

vial::Task<int> async_main () {
    {
        std::vector<int> a;

        for (int i = 0; i < kTestSize; i++) {
            a.push_back(rand() % 1000);
        }

        long long total = 0;
        for (size_t i = 0; i < kRuns; i++) {
            auto start = std::chrono::high_resolution_clock::now();
            co_await merge_sort(&a, 0, a.size());
            auto end = std::chrono::high_resolution_clock::now();
            total += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }

        std::cout << "PAR: " << total / kRuns << "ns" << std::endl;

        for (size_t i = 0; i + 1 < a.size(); i++) {
            assert(a[i] <= a[i+1]);
        }
    }

    {
        std::vector<int> a;

        for (size_t i = 0; i < kTestSize; i++) {
            a.push_back(rand() % 1000);
        }
        
        long long total = 0;
        for (size_t i = 0; i < kRuns; i++) {
            auto start = std::chrono::high_resolution_clock::now();
            sort(a.begin(), a.end());
            auto end = std::chrono::high_resolution_clock::now();
            total += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }

        std::cout << "STD: " << total / kRuns << "ns" << std::endl;

        for (int i = 0; i + 1 < a.size(); i++) {
            assert(a[i] <= a[i+1]);
        }
    }

    co_return 1;
}