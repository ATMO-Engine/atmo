#pragma once

#include <iostream>
#include "lua.h"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace luau
    {

        template <typename T> class LuaBindings
        {
        public:
            static void RegisterType(lua_State *state)
            {
                // TODO: spdlog generate crash here
                return;
            }
        };
    } // namespace luau
} // namespace atmo
