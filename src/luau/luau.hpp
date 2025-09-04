#pragma once

#include "lua.h"
#include "luacode.h"
#include "luaconf.h"
#include "lualib.h"

namespace atmo
{
    namespace luau
    {
        class Luau
        {
            public:
                void init();
                void close();

            protected:
                lua_State *L;
        };
    } // namespace luau
} // namespace atmo
