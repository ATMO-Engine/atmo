#pragma once

#include "core/types.hpp"
#include "lua_bindings.hpp"
#include "lualib.h"

namespace atmo::luau
{
    using namespace atmo::core::types;

    template <> class LuaBindings<Vector2> : public atmo::luau::LuaBindingsBase<atmo::luau::LuaBindings<Vector2>, Vector2>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, GC, "Vector2.__gc");
            lua_setfield(state, -2, "__gc");

            lua_pushcfunction(state, Index, "Vector2.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Vector2.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_pushcfunction(state, Length, "Vector2.length");
            lua_setfield(state, -2, "length");

            lua_setfield(state, -2, "__methods");

            lua_pop(state, 1);

            lua_pushcfunction(state, New, name);
            lua_setglobal(state, name);
        }

        static Property m_properties[];
        static constexpr const char *name = "Vector2";

    private:
        static int New(lua_State *state)
        {
            float x = (float)luaL_checknumber(state, 1);
            float y = (float)luaL_checknumber(state, 2);

            LuaBindings<Vector2>::push(state, new Vector2(x, y), true);
            return 1;
        }

        static int GC(lua_State *state)
        {
            return LuaBindingsBase<LuaBindings<Vector2>, Vector2>::GC(state);
        }

        static int Length(lua_State *state)
        {
            Vector2 *v = check_ptr(state, 1);
            if (!v) {
                return 0;
            }
            lua_pushnumber(state, v->length());
            return 1;
        }
    };
} // namespace atmo::luau
