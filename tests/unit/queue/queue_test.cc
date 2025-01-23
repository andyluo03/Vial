#include <gtest/gtest.h>
#include <set>
#include <iostream>

#include "core/queue.hh"


TEST(QueueTest, SingleThreaded) {
  constexpr int kNumEntries = 100;

  vial::Queue<int> tester;

  for (int i = 0; i < kNumEntries; i++) {
    tester.enqueue(i);
  }

  std::set<int> seen;
  for (int i = 0; i < kNumEntries; i++) {
    int res = tester.try_get().value();
    seen.insert(res);
    std::cerr << res << std::endl;
  }

  ASSERT_EQ(seen.size(), kNumEntries);
}
