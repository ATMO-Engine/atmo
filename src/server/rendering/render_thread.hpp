#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

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
        std::mutex m_mutex;
        std::condition_variable m_condition;
        std::optional<CommandQueue::Batch> m_pending;
        FrameFunction m_frame;
        bool m_busy = false;
        bool m_stop = false;
    };
} // namespace atmo::server::rendering
