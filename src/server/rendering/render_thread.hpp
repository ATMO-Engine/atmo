#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

#include "impl/profiler.hpp"
#include "server/rendering/command_queue.hpp"

namespace atmo::server::rendering
{
    class RenderThread
    {
    public:
        using FrameFunction = std::function<void(CommandQueue::Batch &)>;

        RenderThread() = default;
        ~RenderThread();

        RenderThread(const RenderThread &) = delete;
        RenderThread &operator=(const RenderThread &) = delete;

        void start(FrameFunction frame);
        void submit(CommandQueue::Batch batch);
        void wait();
        void stop();

        [[nodiscard]] bool running() const noexcept;
        [[nodiscard]] std::thread::id id() const noexcept;

    private:
        void loop();

        std::thread m_thread;
        ATMO_PROFILE_LOCKABLE_N(std::mutex, m_mutex, "RenderThread");
        std::condition_variable_any m_condition;
        std::optional<CommandQueue::Batch> m_pending;
        FrameFunction m_frame;
        bool m_busy = false;
        bool m_stop = false;
    };
} // namespace atmo::server::rendering
