#include "render_thread.hpp"

#include <utility>

#include "impl/profiler.hpp"

namespace atmo::server::rendering
{
    RenderThread::~RenderThread()
    {
        stop();
    }

    void RenderThread::start(FrameFunction frame)
    {
        if (m_thread.joinable())
            return;
        m_frame = std::move(frame);
        m_stop = false;
        m_thread = std::thread([this] { loop(); });
    }

    void RenderThread::submit(CommandQueue::Batch batch)
    {
        std::unique_lock lock(m_mutex);
        m_condition.wait(lock, [this] { return !m_busy; });
        m_pending.emplace(std::move(batch));
        m_busy = true;
        m_condition.notify_all();
    }

    void RenderThread::wait()
    {
        std::unique_lock lock(m_mutex);
        m_condition.wait(lock, [this] { return !m_busy; });
    }

    void RenderThread::stop()
    {
        if (!m_thread.joinable())
            return;
        {
            std::unique_lock lock(m_mutex);
            m_condition.wait(lock, [this] { return !m_busy; });
            m_stop = true;
        }
        m_condition.notify_all();
        m_thread.join();
        m_frame = {};
    }

    bool RenderThread::running() const noexcept
    {
        return m_thread.joinable();
    }

    std::thread::id RenderThread::id() const noexcept
    {
        return m_thread.get_id();
    }

    void RenderThread::loop()
    {
        ATMO_PROFILE_THREAD("Render");
        for (;;) {
            CommandQueue::Batch batch;
            {
                std::unique_lock lock(m_mutex);
                m_condition.wait(lock, [this] { return m_stop || m_pending.has_value(); });
                if (!m_pending)
                    return;
                batch = std::move(*m_pending);
                m_pending.reset();
            }

            m_frame(batch);

            {
                std::scoped_lock lock(m_mutex);
                m_busy = false;
            }
            m_condition.notify_all();
        }
    }
} // namespace atmo::server::rendering
