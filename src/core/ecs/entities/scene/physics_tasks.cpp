#include "physics_tasks.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <thread>

#include "core/thread/thread_pool.hpp"

namespace atmo::core::ecs::physics
{
    namespace
    {
        ThreadPool &Pool()
        {
            static ThreadPool pool(std::clamp<std::size_t>(std::thread::hardware_concurrency() / 2, 1, 8));
            return pool;
        }
    } // namespace

    void *EnqueueTask(b2TaskCallback *task, int item_count, int min_range, void *task_context, void *)
    {
        if (item_count <= 0)
            return nullptr;

        auto &pool = Pool();
        const int max_tasks = static_cast<int>(pool.getWorkerCount()) * 2;
        const int range = std::max(min_range, 1);
        const int task_count = std::clamp((item_count + range - 1) / range, 1, max_tasks);
        const int chunk_size = (item_count + task_count - 1) / task_count;

        auto *group = new TaskHandleGroup();
        for (int i = 0; i < task_count; ++i) {
            const int start = i * chunk_size;
            const int end = std::min(start + chunk_size, item_count);
            if (start < end)
                group->tasks.emplace_back(
                    pool.submit([=, &pool] { task(start, end, static_cast<std::uint32_t>(pool.currentWorkerIndex().value()), task_context); }));
        }
        return group;
    }

    void FinishTask(void *user_task, void *)
    {
        auto *group = static_cast<TaskHandleGroup *>(user_task);
        for (auto &task : group->tasks) task.wait();
        delete group;
    }

    int WorkerCount()
    {
        return static_cast<int>(Pool().getWorkerCount());
    }
} // namespace atmo::core::ecs::physics
