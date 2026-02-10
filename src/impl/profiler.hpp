#pragma once

#if defined(ATMO_PROFILING)

    #if defined(ATMO_PROFILER_TRACY)
        #include <tracy/Tracy.hpp>

        #define ATMO_PROFILE_SCOPE() ZoneScoped
        #define ATMO_PROFILE_SCOPE_COLOR(col) ZoneScopedC(col)
        #define ATMO_PROFILE_SCOPE_N(name) ZoneScopedN(name)
        #define ATMO_PROFILE_SCOPE_N_COLOR(name, color) ZoneScopedNC(name, color)
        #define ATMO_PROFILE_ZONE_NAME(name, size) ZoneName(name, size)
        #define ATMO_PROFILE_ZONE_VALUE(val) ZoneValue(val)
        #define ATMO_PROFILE_ZONE_TEXT(text, size) ZoneText(text, size)
        #define ATMO_PROFILE_FRAME() FrameMark
        #define ATMO_PROFILE_THREAD(name) tracy::SetThreadName(name)
        #define ATMO_PROFILE_PLOT(name, value) TracyPlot(name, value)
        #define ATMO_PROFILE_MSG(msg) TracyMessage(msg)

    #else
        #define ATMO_PROFILE_SCOPE()
        #define ATMO_PROFILE_SCOPE_COLOR(col)
        #define ATMO_PROFILE_SCOPE_N(name)
        #define ATMO_PROFILE_SCOPE_N_COLOR(name, color)
        #define ATMO_PROFILE_ZONE_NAME(name, size)
        #define ATMO_PROFILE_ZONE_VALUE(val)
        #define ATMO_PROFILE_ZONE_TEXT_L(text)
        #define ATMO_PROFILE_FRAME()
        #define ATMO_PROFILE_THREAD(name)
        #define ATMO_PROFILE_PLOT(name, value)
        #define ATMO_PROFILE_MSG(msg)
    #endif

#else
    #define ATMO_PROFILE_SCOPE()
    #define ATMO_PROFILE_SCOPE_COLOR(col)
    #define ATMO_PROFILE_SCOPE_N(name)
    #define ATMO_PROFILE_SCOPE_N_COLOR(name, color)
    #define ATMO_PROFILE_ZONE_NAME(name, size)
    #define ATMO_PROFILE_ZONE_VALUE(val)
    #define ATMO_PROFILE_ZONE_TEXT_L(text)
    #define ATMO_PROFILE_FRAME()
    #define ATMO_PROFILE_THREAD(name)
    #define ATMO_PROFILE_PLOT(name, value)
    #define ATMO_PROFILE_MSG(msg)
#endif
