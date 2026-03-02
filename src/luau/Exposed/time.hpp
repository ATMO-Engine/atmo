#pragma once

#include "lua.h"

namespace atmo {
    namespace luau {
        class Time {
            public:
                Time() = delete;
                ~Time() = delete;

                static void RegisterTime(lua_State *state);

                static int TimeWait(lua_State *state);
        };
    } //namespace luau
} // namespace atmo
