#pragma once
#include "core/types.hpp"
#include "lua_bindings.hpp"
#include "lualib.h"

namespace atmo::luau
{

    using namespace atmo::core::types;

    template <> class LuaBindings<Color> : public atmo::luau::LuaBindingsBase<atmo::luau::LuaBindings<Color>, Color>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, GC, "Color.__gc");
            lua_setfield(state, -2, "__gc");

            lua_pushcfunction(state, Index, "Color.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Color.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_setfield(state, -2, "__methods");

            lua_pop(state, 1);

            lua_pushcfunction(state, New, name);
            lua_setglobal(state, name);
        }

        static Property m_properties[];
        static constexpr const char *name = "Color";

    private:
        static int New(lua_State *state)
        {
            float r = (float)luaL_checknumber(state, 1);
            float g = (float)luaL_checknumber(state, 2);
            float b = (float)luaL_checknumber(state, 3);
            float a = (float)luaL_checknumber(state, 4);

            Push(state, new Color(r, g, b, a), true);
            return 1;
        }

        static int GC(lua_State *state)
        {
            return LuaBindingsBase<LuaBindings<Color>, Color>::GC(state);
        }
    };
} // namespace atmo::luau
