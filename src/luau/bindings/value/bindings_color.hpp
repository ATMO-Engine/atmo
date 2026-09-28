#pragma once
#include "core/types.hpp"
#include "lualib.h"
#include "luau/bindings/lua_bindings.hpp"
#include "luau/bindings/value/value_binding_base.hpp"

namespace atmo::luau
{
    using namespace atmo::core::types;

    template <> class LuaBindings<Color> : public ValueLuaBindings<LuaBindings<Color>, Color>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, Index, "Color.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Color.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_setfield(state, -2, "__methods");

            RegisterProperties(state);

            Seal(state);

            lua_pop(state, 1);

            lua_pushcfunction(state, New, name);
            lua_setglobal(state, name);

            spdlog::debug("Color bindings");
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

            Push(state, Color(r, g, b, a));
            return 1;
        }
    };
} // namespace atmo::luau
