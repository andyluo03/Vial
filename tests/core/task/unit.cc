#include <gtest/gtest.h>

#include "core/task.hh"

TEST(TaskUnit, SimpleContinuation) {
  auto bottom = [](int* t) -> vial::Task<int> {
    *t += 1;
    co_return 1;
  };

  auto top = [&bottom](int* t) -> vial::Task<int> {
    co_await bottom(t);
    *t += 1;
    co_return 1;
  };

  int a = 0;
  vial::TaskBase *foo = top(&a).clone();

  EXPECT_EQ(foo->run(), vial::kAwaiting);
  EXPECT_EQ(foo->get_awaiting()->run(), vial::kComplete);
  EXPECT_EQ(foo->run(), vial::kComplete);
  EXPECT_EQ(a, 2);
}

TEST(TaskUnit, ManySimpleContinuations) {
  const int expectedRuns = 16384;
  int actualRuns = 0;

  auto tester = [&actualRuns](auto self, int i) -> vial::Task<int> {
    if (i == 0) {
      co_return 0;
    }

    int res = 1 + co_await self(self, i-1);
    EXPECT_EQ(i, res);
    actualRuns++;
    co_return res;
  };

  vial::TaskBase* foo = tester(tester, expectedRuns).clone();

  while(foo != nullptr) {
    bool to_delete = foo->get_awaiting() != nullptr;

    vial::TaskState state = foo->run();

    if (to_delete) {
      foo->get_awaiting()->destroy();
    }
    
    if (state == vial::kAwaiting) {
      auto *copy = foo;
      foo = foo->get_awaiting();
      foo->set_callback(copy);
    } else {
      if (foo->get_callback() == nullptr) {
        foo->destroy();
      } 
      foo = foo->get_callback();
    }
  }

  EXPECT_EQ(expectedRuns, actualRuns);
}

TEST(TaskUnit, MultipleCallbacks) {
  const int expectedCalls = 4096;
  int actualCalls = 0;

  auto callee = []() -> vial::Task<int> {
    co_return 1;
  };

  auto caller = [&callee, &actualCalls]() -> vial::Task<int> {
    int result = 0;
    for (int i = 0; i < expectedCalls; i++) {
      result += co_await callee();
    }

    actualCalls = result;
    co_return result;
  };

  vial::TaskBase* foo = caller().clone();

  while(foo->get_state() != vial::kComplete) {
    vial::TaskBase* to_destroy = foo->get_callback();

    vial::TaskState state = foo->run();

    if (to_destroy != nullptr) {
      to_destroy->destroy();
    }

    if (state == vial::kAwaiting) {
      foo->get_awaiting()->run();
    }
  }

  EXPECT_EQ(actualCalls, expectedCalls);
  foo->destroy();
}

TEST(TaskUnit, EnqueuedBehavior) {
  auto tester = []() -> vial::Task<int> {
    co_return 1;
  };

  vial::TaskBase* foo = tester().clone();

  EXPECT_FALSE(foo->is_enqueued());

  foo->set_enqueued_true();
  EXPECT_TRUE(foo->is_enqueued());

  foo->set_enqueued_false();
  EXPECT_FALSE(foo->is_enqueued());

  foo->set_enqueued_true();
  EXPECT_TRUE(foo->is_enqueued());
}
