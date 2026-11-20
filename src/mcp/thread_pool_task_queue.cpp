#include "thread_pool_task_queue.hpp"

#include <algorithm>

namespace atmo::mcp
{
    bool ThreadPoolTaskQueue::enqueue(std::function<void()> fn)
    {
        std::lock_guard lock(m_mutex);
        if (m_shutdown)
            return false;
        std::erase_if(m_tasks, [](const core::TaskHandle &task) { return task.isDone(); });
        m_tasks.push_back(core::ThreadPool::Instance().submit(std::move(fn)));
        return true;
    }

    void ThreadPoolTaskQueue::shutdown()
    {
        std::vector<core::TaskHandle> tasks;
        {
            std::lock_guard lock(m_mutex);
            m_shutdown = true;
            tasks.swap(m_tasks);
        }
        for (auto &task : tasks) task.wait();
    }
} // namespace atmo::mcp
