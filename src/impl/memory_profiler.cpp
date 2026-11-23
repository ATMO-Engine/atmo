#include "memory_profiler.hpp"

#include "impl/profiler.hpp"

#if defined(_WIN32)
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <windows.h>
// windows.h must come first
#include <malloc.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <malloc/malloc.h>
#elif defined(__linux__)
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#endif

#if defined(ATMO_PROFILING) && defined(ATMO_PROFILER_TRACY)

#include <atomic>
#include <cstdlib>
#include <new>

// The sanitizers ship their own operator new/delete; replacing them would hide their mismatch checks.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define ATMO_NO_HEAP_TRACKING
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define ATMO_NO_HEAP_TRACKING
#endif
#endif

#if !defined(ATMO_NO_HEAP_TRACKING) && (defined(_WIN32) || defined(__APPLE__) || defined(__linux__))
#define ATMO_HEAP_TRACKING
#endif

#endif

namespace atmo::profiler::memory
{
    std::size_t ProcessResidentBytes()
    {
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS counters{};
        if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
            return counters.WorkingSetSize;
        return 0;
#elif defined(__APPLE__)
        mach_task_basic_info info{};
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
            return info.resident_size;
        return 0;
#elif defined(__linux__)
        // /proc/self/statm: "size resident shared ..." in pages. Plain read() so the probe itself does not allocate.
        const int fd = ::open("/proc/self/statm", O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            return 0;
        char buffer[128];
        const ssize_t length = ::read(fd, buffer, sizeof(buffer) - 1);
        ::close(fd);
        if (length <= 0)
            return 0;
        buffer[length] = '\0';

        const char *cursor = buffer;
        while (*cursor != '\0' && *cursor != ' ') ++cursor; // skip "size"
        std::size_t resident_pages = 0;
        while (*cursor == ' ') ++cursor;
        while (*cursor >= '0' && *cursor <= '9') resident_pages = resident_pages * 10 + static_cast<std::size_t>(*cursor++ - '0');
        return resident_pages * static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
#else
        return 0;
#endif
    }

#if defined(ATMO_HEAP_TRACKING)
    namespace
    {
        // Constant-initialized: safe to touch from allocations made during static initialization.
        constinit std::atomic<std::int64_t> s_live_bytes{ 0 };
        constinit std::atomic<std::int64_t> s_frame_bytes{ 0 };
        constinit std::atomic<std::int64_t> s_frame_count{ 0 };

        std::size_t UsableSize(void *ptr) noexcept
        {
#if defined(_WIN32)
            return _msize(ptr);
#elif defined(__APPLE__)
            return malloc_size(ptr);
#else
            return malloc_usable_size(ptr);
#endif
        }
    } // namespace

    // Not in the anonymous namespace: the global operator new/delete below call them.
    void *TrackedAlloc(std::size_t size) noexcept;
    void TrackedFree(void *ptr) noexcept;

    void *TrackedAlloc(std::size_t size) noexcept
    {
        void *ptr = std::malloc(size != 0 ? size : 1);
        if (ptr) {
            const auto usable = static_cast<std::int64_t>(UsableSize(ptr));
            s_live_bytes.fetch_add(usable, std::memory_order_relaxed);
            s_frame_bytes.fetch_add(usable, std::memory_order_relaxed);
            s_frame_count.fetch_add(1, std::memory_order_relaxed);
        }
        return ptr;
    }

    void TrackedFree(void *ptr) noexcept
    {
        if (!ptr)
            return;
        s_live_bytes.fetch_sub(static_cast<std::int64_t>(UsableSize(ptr)), std::memory_order_relaxed);
        std::free(ptr);
    }
#endif

    void PlotFrame()
    {
#if defined(ATMO_PROFILING) && defined(ATMO_PROFILER_TRACY)
        [[maybe_unused]] static const bool configured = [] {
            ATMO_PROFILE_PLOT_MEMORY(plot::ProcessMemory);
            ATMO_PROFILE_PLOT_MEMORY(plot::HeapLive);
            ATMO_PROFILE_PLOT_MEMORY(plot::HeapAllocatedPerFrame);
            ATMO_PROFILE_PLOT_MEMORY(plot::LuauHeap);
            ATMO_PROFILE_PLOT_MEMORY(plot::GpuMemory);
            ATMO_PROFILE_PLOT_MEMORY(plot::GpuBudget);
            return true;
        }();

        ATMO_PROFILE_PLOT(plot::ProcessMemory, static_cast<std::int64_t>(ProcessResidentBytes()));

#if defined(ATMO_HEAP_TRACKING)
        ATMO_PROFILE_PLOT(plot::HeapLive, s_live_bytes.load(std::memory_order_relaxed));
        ATMO_PROFILE_PLOT(plot::HeapAllocatedPerFrame, s_frame_bytes.exchange(0, std::memory_order_relaxed));
        ATMO_PROFILE_PLOT(plot::AllocationsPerFrame, s_frame_count.exchange(0, std::memory_order_relaxed));
#endif
#endif
    }
} // namespace atmo::profiler::memory

#if defined(ATMO_HEAP_TRACKING)
// Replaceable global allocation functions (non-aligned forms). The aligned forms keep the standard library
// implementation, which pairs its own aligned allocation with its own aligned deallocation.
namespace
{
    void *NewOrThrow(std::size_t size)
    {
        if (void *ptr = atmo::profiler::memory::TrackedAlloc(size))
            return ptr;
        throw std::bad_alloc();
    }
} // namespace

void *operator new(std::size_t size)
{
    return NewOrThrow(size);
}

void *operator new[](std::size_t size)
{
    return NewOrThrow(size);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept
{
    return atmo::profiler::memory::TrackedAlloc(size);
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
    return atmo::profiler::memory::TrackedAlloc(size);
}

void operator delete(void *ptr) noexcept
{
    atmo::profiler::memory::TrackedFree(ptr);
}

void operator delete[](void *ptr) noexcept
{
    atmo::profiler::memory::TrackedFree(ptr);
}

void operator delete(void *ptr, std::size_t) noexcept
{
    atmo::profiler::memory::TrackedFree(ptr);
}

void operator delete[](void *ptr, std::size_t) noexcept
{
    atmo::profiler::memory::TrackedFree(ptr);
}

void operator delete(void *ptr, const std::nothrow_t &) noexcept
{
    atmo::profiler::memory::TrackedFree(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t &) noexcept
{
    atmo::profiler::memory::TrackedFree(ptr);
}
#endif
