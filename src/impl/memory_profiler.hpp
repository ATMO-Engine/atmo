#pragma once

#include <cstddef>

namespace atmo::profiler::memory
{
    /// Resident memory of the whole process in bytes (RAM actually used, all allocators and drivers included). 0 if unavailable.
    std::size_t ProcessResidentBytes();

    /**
     * Pushes the per-frame memory plots (process RSS, C++ heap live / allocated / allocation count).
     * Call once per main-loop frame. No-op unless profiling is enabled.
     *
     * The C++ heap figures come from a global operator new/delete replacement that is only compiled in profiling builds
     * without ASan/TSan (the sanitizers replace operator new themselves). They cover `new`/`delete` only: C libraries
     * calling malloc directly (SDL, flecs, Luau, Box2D) are tracked by their own plots or by the process RSS.
     */
    void PlotFrame();
} // namespace atmo::profiler::memory

#if defined(ATMO_PROFILING) && defined(ATMO_PROFILER_TRACY)
#define ATMO_PROFILE_MEMORY_FRAME() ::atmo::profiler::memory::PlotFrame()
#else
#define ATMO_PROFILE_MEMORY_FRAME()
#endif
