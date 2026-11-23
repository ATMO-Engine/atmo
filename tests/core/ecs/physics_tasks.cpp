#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "box2d/box2d.h"
#include "core/ecs/entities/scene/physics_tasks.hpp"
#include "core/thread/thread_pool.hpp"

using namespace std::chrono_literals;

namespace
{
    struct TaskProbe {
        std::unique_ptr<std::atomic<bool>[]> busy;
        std::atomic<int> items{ 0 };
        std::atomic<int> out_of_range{ 0 };
        std::atomic<int> shared{ 0 };
        std::atomic<int> on_engine_pool{ 0 };
        std::uint32_t workers = 0;
    };

    void ProbeTask(int start, int end, std::uint32_t worker, void *context)
    {
        auto &probe = *static_cast<TaskProbe *>(context);
        if (atmo::core::ThreadPool::Instance().currentWorkerIndex())
            ++probe.on_engine_pool;
        if (worker >= probe.workers) {
            ++probe.out_of_range;
            return;
        }
        if (probe.busy[worker].exchange(true))
            ++probe.shared;
        std::this_thread::sleep_for(200us);
        probe.items += end - start;
        probe.busy[worker] = false;
    }
} // namespace

TEST_CASE("Box2D tasks run on their own pool with the index of the worker running them", "[physics]")
{
    using namespace atmo::core::ecs;

    TaskProbe probe;
    probe.workers = static_cast<std::uint32_t>(physics::WorkerCount());
    probe.busy = std::make_unique<std::atomic<bool>[]>(probe.workers);

    for (int round = 0; round < 20; ++round) {
        void *task = physics::EnqueueTask(&ProbeTask, 10000, 1, &probe, nullptr);
        REQUIRE(task != nullptr);
        physics::FinishTask(task, nullptr);
    }

    REQUIRE(probe.out_of_range == 0);
    REQUIRE(probe.shared == 0);
    REQUIRE(probe.on_engine_pool == 0);
    REQUIRE(probe.items == 20 * 10000);
    REQUIRE(physics::EnqueueTask(&ProbeTask, 0, 1, &probe, nullptr) == nullptr);
}

TEST_CASE("A Box2D world with many bodies steps while every engine pool worker is busy", "[physics]")
{
    using namespace atmo::core::ecs;

    auto &engine_pool = atmo::core::ThreadPool::Instance();
    const std::size_t engine_workers = engine_pool.getWorkerCount();
    std::atomic<std::size_t> holding{ 0 };
    std::atomic<bool> release{ false };
    std::vector<atmo::core::TaskHandle> held;
    for (std::size_t i = 0; i < engine_workers; ++i)
        held.push_back(engine_pool.submit([&] {
            ++holding;
            while (!release) std::this_thread::sleep_for(1ms);
        }));
    const auto hold_deadline = std::chrono::steady_clock::now() + 5s;
    while (holding < engine_workers && std::chrono::steady_clock::now() < hold_deadline) std::this_thread::sleep_for(1ms);
    if (holding != engine_workers) {
        release = true;
        for (auto &task : held) task.wait();
        FAIL("Could not occupy every engine pool worker");
    }

    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = { 0.0f, 9.81f };
    world_def.workerCount = physics::WorkerCount();
    world_def.enqueueTask = physics::EnqueueTask;
    world_def.finishTask = physics::FinishTask;
    const b2WorldId world = b2CreateWorld(&world_def);

    b2BodyDef ground_def = b2DefaultBodyDef();
    ground_def.position = { 0.0f, 12.0f };
    const b2BodyId ground = b2CreateBody(world, &ground_def);
    const b2ShapeDef shape_def = b2DefaultShapeDef();
    const b2Polygon ground_box = b2MakeBox(40.0f, 0.5f);
    b2CreatePolygonShape(ground, &shape_def, &ground_box);

    std::vector<b2BodyId> bodies;
    const b2Circle circle{ .center = { 0.0f, 0.0f }, .radius = 0.2f };
    for (int i = 0; i < 800; ++i) {
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_dynamicBody;
        body_def.position = { static_cast<float>(i % 40) * 0.45f - 9.0f, static_cast<float>(i / 40) * -0.45f };
        bodies.push_back(b2CreateBody(world, &body_def));
        b2CreateCircleShape(bodies.back(), &shape_def, &circle);
    }

    const float start_y = b2Body_GetPosition(bodies.front()).y;
    for (int step = 0; step < 120; ++step) b2World_Step(world, 1.0f / 60.0f, 4);
    const float end_y = b2Body_GetPosition(bodies.front()).y;
    b2DestroyWorld(world);

    release = true;
    for (auto &task : held) task.wait();

    REQUIRE(end_y > start_y + 1.0f);
}
