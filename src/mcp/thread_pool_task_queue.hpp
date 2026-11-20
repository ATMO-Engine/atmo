#pragma once

#include <functional>
#include <mutex>
#include <vector>

#include "core/thread/thread_pool.hpp"
#include "httplib.h"

namespace atmo::mcp
{
    class ThreadPoolTaskQueue final : public httplib::TaskQueue
    {
    public:
        bool enqueue(std::function<void()> fn) override;
        void shutdown() override;

    private:
        std::mutex m_mutex;
        std::vector<core::TaskHandle> m_tasks;
        bool m_shutdown = false;
    };
} // namespace atmo::mcp
