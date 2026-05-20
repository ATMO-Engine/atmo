#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/thread/thread_pool.hpp"

using namespace std::chrono_literals;

static bool spinWait(const std::atomic<int> &counter, int target, std::chrono::milliseconds timeout = 3000ms)
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (counter.load(std::memory_order_acquire) < target) {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(1ms);
    }
    return true;
}

TEST_CASE("submit runs a task exactly once", "[thread_pool]")
{
    std::atomic<int> count{ 0 };
    atmo::core::ThreadPool::Instance().submit([&count] { count.fetch_add(1, std::memory_order_release); });

    REQUIRE(spinWait(count, 1));
    std::this_thread::sleep_for(50ms);
    REQUIRE(count.load() == 1);
}

TEST_CASE("repeat runs a task the requested number of times", "[thread_pool]")
{
    std::atomic<int> count{ 0 };
    auto handle = atmo::core::ThreadPool::Instance().repeat([&count] { count.fetch_add(1, std::memory_order_release); }, 5);

    REQUIRE(spinWait(count, 5));
    std::this_thread::sleep_for(50ms);
    REQUIRE(count.load() == 5);
}

TEST_CASE("repeat -1 runs a task indefinitely until cancelled", "[thread_pool]")
{
    std::atomic<int> count{ 0 };
    auto handle = atmo::core::ThreadPool::Instance().repeat([&count] { count.fetch_add(1, std::memory_order_release); }, -1);

    REQUIRE(spinWait(count, 20));
    handle.cancel();
    REQUIRE(handle.isCancelled());

    int snapshot = count.load();
    std::this_thread::sleep_for(100ms);
    REQUIRE(count.load() < snapshot + 5);
}

TEST_CASE("cancelled task stops executing", "[thread_pool]")
{
    std::atomic<int> count{ 0 };
    auto handle = atmo::core::ThreadPool::Instance().repeat([&count] { count.fetch_add(1, std::memory_order_release); }, -1);

    REQUIRE(spinWait(count, 5));
    handle.cancel();

    std::this_thread::sleep_for(100ms);
    int snapshot = count.load();
    std::this_thread::sleep_for(100ms);

    REQUIRE(count.load() == snapshot);
}

TEST_CASE("repeat with delay spaces out executions", "[thread_pool]")
{
    constexpr auto delay = 50ms;
    constexpr int runs = 3;

    std::atomic<int> count{ 0 };
    auto t0 = std::chrono::steady_clock::now();

    auto handle = atmo::core::ThreadPool::Instance().repeat([&count] { count.fetch_add(1, std::memory_order_release); }, runs, delay);

    REQUIRE(spinWait(count, runs, 2000ms));

    auto elapsed = std::chrono::steady_clock::now() - t0;
    REQUIRE(elapsed >= (runs - 1) * delay);
}

TEST_CASE("many concurrent submits all complete", "[thread_pool]")
{
    constexpr int N = 500;
    std::atomic<int> count{ 0 };

    for (int i = 0; i < N; ++i) atmo::core::ThreadPool::Instance().submit([&count] { count.fetch_add(1, std::memory_order_release); });

    REQUIRE(spinWait(count, N, 5000ms));
    REQUIRE(count.load() == N);
}

TEST_CASE("submits from multiple threads all complete", "[thread_pool]")
{
    constexpr int producers = 8;
    constexpr int per_producer = 50;
    constexpr int total = producers * per_producer;
    std::atomic<int> count{ 0 };

    {
        std::vector<std::jthread> threads;
        threads.reserve(producers);
        for (int i = 0; i < producers; ++i) {
            threads.emplace_back([&count] {
                for (int j = 0; j < per_producer; ++j) atmo::core::ThreadPool::Instance().submit([&count] { count.fetch_add(1, std::memory_order_release); });
            });
        }
    }

    REQUIRE(spinWait(count, total, 5000ms));
    REQUIRE(count.load() == total);
}
