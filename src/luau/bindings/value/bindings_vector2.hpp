#pragma once

#include "core/types.hpp"
#include "lualib.h"
#include "luau/bindings/lua_bindings.hpp"
#include "luau/bindings/value/value_binding_base.hpp"

namespace atmo::luau
{
    using namespace atmo::core::types;

    template <> class LuaBindings<Vector2> : public ValueLuaBindings<LuaBindings<Vector2>, Vector2>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, Index, "Vector2.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Vector2.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_pushcfunction(state, Length, "Vector2.length");
            lua_setfield(state, -2, "length");

            lua_setfield(state, -2, "__methods");

            RegisterProperties(state);

            Seal(state);

            lua_pop(state, 1);

            lua_pushcfunction(state, New, name);
            lua_setglobal(state, name);

            spdlog::debug("Vector bindings");
        }

        static Property m_properties[];
        static constexpr const char *name = "Vector2";

    private:
        static int New(lua_State *state)
        {
            float x = (float)luaL_checknumber(state, 1);
            float y = (float)luaL_checknumber(state, 2);

            Push(state, Vector2(x, y));
            return 1;
        }

        static int Length(lua_State *state)
        {
            Vector2 *v = CheckPtr(state, 1);
            if (!v) {
                return 0;
            }
            lua_pushnumber(state, v->length());
            return 1;
        }
    };
} // namespace atmo::luau
