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
        {
            // Time the caller spends blocked because the previous frame is still rendering.
            ATMO_PROFILE_SCOPE_N_COLOR("RenderThread::submit wait", profiler::color::Wait);
            m_condition.wait(lock, [this] { return !m_busy; });
        }
        m_pending.emplace(std::move(batch));
        m_busy = true;
        m_condition.notify_all();
    }

    void RenderThread::wait()
    {
        ATMO_PROFILE_SCOPE_N_COLOR("RenderThread::wait", profiler::color::Wait);
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
        ATMO_PROFILE_THREAD_GROUP("Render", profiler::group::Render);
        ATMO_PROFILE_PLOT_STEP(profiler::PlotDrawCalls);
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

            {
                ATMO_PROFILE_SCOPE_N_COLOR("RenderThread::frame", profiler::color::Render);
                m_frame(batch);
            }
            // Secondary frame set: render-thread frame time, independent from the main loop's FrameMark.
            ATMO_PROFILE_FRAME_N("Render");
            ATMO_PROFILE_DRAW_CALLS_FLUSH();

            {
                std::scoped_lock lock(m_mutex);
                m_busy = false;
            }
            m_condition.notify_all();
        }
    }
} // namespace atmo::server::rendering
