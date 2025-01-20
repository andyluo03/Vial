#include <gtest/gtest.h>
#include "core/engine.hh"
#include <thread>
#include <chrono>
#include <iostream>

using namespace vial;

Task<int> simple_task() {
    co_return 42;
}

Task<int> chained_task(int n) {
    int result = co_await simple_task();
    co_return result + n;
}

class FiberTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& scheduler = Scheduler::instance();
        scheduler.reset();
    }

    void TearDown() override {
        auto& scheduler = Scheduler::instance();
        scheduler.reset();
    }
};

TEST_F(FiberTest, SimpleTask) {
    auto fiber = Fiber<int>::create([]() { return simple_task(); });
    auto state = fiber.get_shared_state();
    EXPECT_FALSE(fiber.is_complete());
    
    auto& scheduler = Scheduler::instance();
    scheduler.spawn(std::move(fiber));
    
    std::thread scheduler_thread([&scheduler]() {
        scheduler.run();
    });
    
    {
        std::unique_lock<std::mutex> lock(state->lock_);
        state->complete_.wait(lock, [&state]() { return state->is_done_; });
        EXPECT_EQ(state->result_, 42);
    }
    
    scheduler.shutdown();
    scheduler_thread.join();
}

TEST_F(FiberTest, ChainedTasks) {
    auto fiber = Fiber<int>::create([]() { return chained_task(8); });
    auto state = fiber.get_shared_state();
    
    auto& scheduler = Scheduler::instance();
    scheduler.spawn(std::move(fiber));
    
    std::thread scheduler_thread([&scheduler]() {
        scheduler.run();
    });
    
    {
        std::unique_lock<std::mutex> lock(state->lock_);
        state->complete_.wait(lock, [&state]() { return state->is_done_; });
        EXPECT_EQ(state->result_, 50);  // 42 + 8
    }
    
    scheduler.shutdown();
    scheduler_thread.join();
} 