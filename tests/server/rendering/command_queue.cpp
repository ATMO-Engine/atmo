#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "server/rendering/command_queue.hpp"

using atmo::server::rendering::CommandQueue;

TEST_CASE("deferred commands run on flush, in submission order", "[rendering][command_queue]")
{
    CommandQueue queue;
    std::vector<int> order;

    for (int i = 0; i < 1000; ++i) queue.push([&order, i] { order.push_back(i); });

    REQUIRE(order.empty());
    REQUIRE(queue.pendingCount() == 1000);

    queue.flush();

    REQUIRE(queue.pendingCount() == 0);
    REQUIRE(order.size() == 1000);
    for (int i = 0; i < 1000; ++i) REQUIRE(order[i] == i);
}

TEST_CASE("inline commands run immediately", "[rendering][command_queue]")
{
    CommandQueue queue(CommandQueue::Mode::Inline);
    int value = 0;

    queue.push([&value] { value = 42; });

    REQUIRE(value == 42);
    REQUIRE(queue.pendingCount() == 0);
}

TEST_CASE("switching to inline mode flushes pending commands first", "[rendering][command_queue]")
{
    CommandQueue queue;
    std::vector<int> order;

    queue.push([&order] { order.push_back(1); });
    queue.setMode(CommandQueue::Mode::Inline);
    queue.push([&order] { order.push_back(2); });

    REQUIRE(order == std::vector<int>{ 1, 2 });
}

TEST_CASE("commands own their captures until they run", "[rendering][command_queue]")
{
    CommandQueue queue;
    std::string result;
    auto shared = std::make_shared<int>(7);

    {
        std::string text = "frame";
        queue.push([&result, text, shared] { result = text + std::to_string(*shared); });
    }

    REQUIRE(shared.use_count() == 2);
    queue.flush();

    REQUIRE(result == "frame7");
    REQUIRE(shared.use_count() == 1);
}

TEST_CASE("pending commands are destroyed without running when the queue dies", "[rendering][command_queue]")
{
    auto shared = std::make_shared<int>(0);
    bool ran = false;

    {
        CommandQueue queue;
        queue.push([shared, &ran] { ran = true; });
        REQUIRE(shared.use_count() == 2);
    }

    REQUIRE_FALSE(ran);
    REQUIRE(shared.use_count() == 1);
}

TEST_CASE("commands pushed while flushing run in the same flush, after the current batch", "[rendering][command_queue]")
{
    CommandQueue queue;
    std::vector<int> order;

    queue.push([&] {
        order.push_back(1);
        queue.push([&order] { order.push_back(3); });
    });
    queue.push([&order] { order.push_back(2); });

    queue.flush();

    REQUIRE(order == std::vector<int>{ 1, 2, 3 });
    REQUIRE(queue.pendingCount() == 0);
}

TEST_CASE("a throwing command does not stop the rest of the batch", "[rendering][command_queue]")
{
    CommandQueue queue;
    std::vector<int> order;

    queue.push([&order] { order.push_back(1); });
    queue.push([] { throw std::runtime_error("expected failure"); });
    queue.push([&order] { order.push_back(3); });

    queue.flush();

    REQUIRE(order == std::vector<int>{ 1, 3 });
}

TEST_CASE("large and over-aligned commands are stored correctly", "[rendering][command_queue]")
{
    CommandQueue queue(CommandQueue::Mode::Deferred, 256);

    struct alignas(64) Aligned {
        std::array<std::byte, 1024> payload{};
    };

    Aligned big;
    big.payload[1023] = std::byte{ 9 };
    bool aligned = false;
    std::byte last{};

    for (int i = 0; i < 8; ++i)
        queue.push([big, &aligned, &last] {
            aligned = reinterpret_cast<std::uintptr_t>(&big) % 64 == 0;
            last = big.payload[1023];
        });

    queue.flush();

    REQUIRE(aligned);
    REQUIRE(last == std::byte{ 9 });
}
