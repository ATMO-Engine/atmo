#pragma once

/**
 * Profiling macros. Everything compiles to nothing (or to the plain type) unless ATMO_PROFILING and ATMO_PROFILER_TRACY are defined.
 *
 * Locks: declare a profiled mutex with ATMO_PROFILE_LOCKABLE(std::mutex, m_mutex) and lock it with CTAD (std::lock_guard lock(m_mutex)).
 * A condition variable waiting on it must be std::condition_variable_any.
 * ATMO_PROFILE_LOCKABLE_TYPE(std::mutex) names the resulting type when it has to be spelled out.
 */

#if defined(ATMO_PROFILING) && defined(ATMO_PROFILER_TRACY)
#include <tracy/Tracy.hpp>

#define ATMO_PROFILE_SCOPE() ZoneScoped
#define ATMO_PROFILE_SCOPE_COLOR(col) ZoneScopedC(col)
#define ATMO_PROFILE_SCOPE_N(name) ZoneScopedN(name)
#define ATMO_PROFILE_SCOPE_N_COLOR(name, color) ZoneScopedNC(name, color)
#define ATMO_PROFILE_ZONE_NAME(name, size) ZoneName(name, size)
#define ATMO_PROFILE_ZONE_VALUE(val) ZoneValue(val)
#define ATMO_PROFILE_ZONE_TEXT(text, size) ZoneText(text, size)
#define ATMO_PROFILE_FRAME() FrameMark
#define ATMO_PROFILE_FRAME_N(name) FrameMarkNamed(name)
#define ATMO_PROFILE_THREAD(name) tracy::SetThreadName(name)
#define ATMO_PROFILE_THREAD_GROUP(name, group) tracy::SetThreadNameWithHint(name, group)
#define ATMO_PROFILE_PLOT(name, value) TracyPlot(name, value)
#define ATMO_PROFILE_PLOT_STEP(name) TracyPlotConfig(name, tracy::PlotFormatType::Number, true, true, 0)
#define ATMO_PROFILE_MSG(msg) TracyMessageL(msg)
#define ATMO_PROFILE_LOCKABLE(type, var) TracyLockable(type, var)
#define ATMO_PROFILE_LOCKABLE_N(type, var, desc) TracyLockableN(type, var, desc)
#define ATMO_PROFILE_LOCKABLE_TYPE(type) LockableBase(type)
#define ATMO_PROFILE_LOCKABLE_NAME(var, name, size) LockableName(var, name, size)

#include <atomic>
#include <cstdint>

namespace atmo::profiler
{
    /// GPU draw calls recorded since the last ATMO_PROFILE_DRAW_CALLS_FLUSH (atomic: command lists may be recorded off the render thread).
    inline std::atomic<std::int64_t> draw_calls{ 0 };
    inline constexpr const char *PlotDrawCalls = "Draw calls";
} // namespace atmo::profiler

#define ATMO_PROFILE_DRAW_CALL() ::atmo::profiler::draw_calls.fetch_add(1, std::memory_order_relaxed)
#define ATMO_PROFILE_DRAW_CALLS_FLUSH()                                                                                                                        \
    TracyPlot(::atmo::profiler::PlotDrawCalls, ::atmo::profiler::draw_calls.exchange(0, std::memory_order_relaxed))

#else

#define ATMO_PROFILE_SCOPE()
#define ATMO_PROFILE_SCOPE_COLOR(col)
#define ATMO_PROFILE_SCOPE_N(name)
#define ATMO_PROFILE_SCOPE_N_COLOR(name, color)
#define ATMO_PROFILE_ZONE_NAME(name, size)
#define ATMO_PROFILE_ZONE_VALUE(val)
#define ATMO_PROFILE_ZONE_TEXT(text, size)
#define ATMO_PROFILE_FRAME()
#define ATMO_PROFILE_FRAME_N(name)
#define ATMO_PROFILE_THREAD(name)
#define ATMO_PROFILE_THREAD_GROUP(name, group)
#define ATMO_PROFILE_PLOT(name, value)
#define ATMO_PROFILE_PLOT_STEP(name)
#define ATMO_PROFILE_MSG(msg)
#define ATMO_PROFILE_LOCKABLE(type, var) type var
#define ATMO_PROFILE_LOCKABLE_N(type, var, desc) type var
#define ATMO_PROFILE_LOCKABLE_TYPE(type) type
#define ATMO_PROFILE_LOCKABLE_NAME(var, name, size)
#define ATMO_PROFILE_DRAW_CALL()
#define ATMO_PROFILE_DRAW_CALLS_FLUSH()

#endif

namespace atmo::profiler
{
    /// Zone colors per subsystem (0xRRGGBB), so the timeline reads at a glance.
    namespace color
    {
        inline constexpr unsigned Task = 0x4CAF50;
        inline constexpr unsigned Wait = 0xE53935;
        inline constexpr unsigned Render = 0x1E88E5;
        inline constexpr unsigned Io = 0xFB8C00;
        inline constexpr unsigned Physics = 0x8E24AA;
    } // namespace color

    /// Thread group hints: threads sharing a hint are listed together in the viewer.
    namespace group
    {
        inline constexpr int Main = 0;
        inline constexpr int Render = 1;
        inline constexpr int Workers = 2;
        inline constexpr int Scheduler = 3;
    } // namespace group
} // namespace atmo::profiler
