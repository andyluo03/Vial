#include <gtest/gtest.h>
#include <set>
#include <iostream>

#include "core/queue.hh"
#include "core/task.hh"

vial::Task<int> dummy_function () {
  co_return 1;
}

TEST(TaskUnit, SetEnqueued) {
  vial::TaskBase* foo = new vial::Task<int>(dummy_function());
  ASSERT_FALSE(foo->enqueued());
  foo->set_enqueued_true();
  ASSERT_TRUE(foo->enqueued());
}
