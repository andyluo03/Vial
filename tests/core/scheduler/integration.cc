#include <gtest/gtest.h>
#include <numeric>

#include "core/scheduler.hh"
#include "core/task.hh"

auto merge_sort (std::vector<int>& a, vial::Scheduler& scheduler, int left, int right, bool top = false) -> vial::Task<int> { //NOLINT
    const int small = 8;

    auto mid = std::midpoint(left, right);
    auto size = right - left;
    
    if (size < small) {
        std::sort(a.begin() + left, a.begin() + right);
        if (top) { scheduler.stop(); }
        co_return 1;
    }

    auto lt = scheduler.spawn_task( merge_sort(a, scheduler, left, mid) );
    auto rt = scheduler.spawn_task( merge_sort(a, scheduler, mid, right) );

    assert(co_await lt);
    assert(co_await rt);

    std::vector<int> merged; merged.reserve(size);

    int l_itr = left;
    int m_itr = mid;

    while (l_itr < mid && m_itr < right) { merged.push_back((a[l_itr] < a[m_itr]) ? a[l_itr++] : a[m_itr++]); }
    while (l_itr < mid) { merged.push_back(a[l_itr++]); }
    while (m_itr < right) { merged.push_back(a[m_itr++]); }

    std::copy(merged.begin(), merged.end(), a.begin() + left);

    if (top) { scheduler.stop(); }
    co_return 1;
}

TEST(SchedulerIntegration, MergeSortSmallSingleThreaded) {
    std::vector<int> base = { 5, 4, 6, 7, 8, 9 }; // NOLINT

    const std::vector<int> expected = { 4, 5, 6, 7, 8, 9 };

    vial::Scheduler scheduler{1};
    scheduler.spawn_task(merge_sort(base, scheduler, 0, (int) base.size(), true));
    scheduler.start();

    ASSERT_EQ(expected.size(), base.size());

    for (int i = 0; i < base.size(); i++) {
        EXPECT_EQ(expected[i], base[i]);
    }
}

TEST(SchedulerIntegration, MergeSortLargeSingleThreaded) {
    const int largeSize = 1e6;

    std::vector<int> base;
    base.reserve(largeSize);
    for (int i = 0; i < largeSize; i++) { 
        base.push_back(rand()); 
    }

    std::vector<int> expected (base.begin(), base.end());
    sort(expected.begin(), expected.end());

    vial::Scheduler scheduler{1};
    scheduler.spawn_task(merge_sort(base, scheduler, 0, (int) base.size(), true));    scheduler.start();

    ASSERT_EQ(expected.size(), base.size());

    bool equal = true;
    for (int i = 0; i < base.size(); i++) {
        if (expected[i] != base[i]) {
            equal = false;
        }
    }

   EXPECT_TRUE(equal);
}

TEST(SchedulerIntegration, MergeSortSmallMultiThreaded) {
    std::vector<int> base = { 5, 4, 6, 7, 8, 9 }; // NOLINT

    const std::vector<int> expected = { 4, 5, 6, 7, 8, 9 };

    vial::Scheduler scheduler;
    scheduler.spawn_task(merge_sort(base, scheduler, 0, (int) base.size(), true));
    scheduler.start();

    ASSERT_EQ(expected.size(), base.size());

    for (int i = 0; i < base.size(); i++) {
        EXPECT_EQ(expected[i], base[i]);
    }
}

TEST(SchedulerIntegration, MergeSortLargeMultiThreaded) {
    const int largeSize = 1e6;

    std::vector<int> base;
    base.reserve(largeSize);
    for (int i = 0; i < largeSize; i++) { 
        base.push_back(rand()); 
    }

    std::vector<int> expected (base.begin(), base.end());
    sort(expected.begin(), expected.end());

    vial::Scheduler scheduler;
    scheduler.spawn_task(merge_sort(base, scheduler, 0, (int) base.size(), true));    scheduler.start();

    ASSERT_EQ(expected.size(), base.size());

    bool equal = true;
    for (int i = 0; i < base.size(); i++) {
        if (expected[i] != base[i]) {
            equal = false;
        }
    }

    EXPECT_TRUE(equal);
}
