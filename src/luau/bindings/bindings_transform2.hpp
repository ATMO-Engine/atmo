#pragma once

#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/types.hpp"
#include "lua_bindings.hpp"
#include "lualib.h"

namespace atmo::luau
{
    using namespace atmo::core::components;

    template <> class LuaBindings<Transform2d> : public LuaBindingsBase<LuaBindings<Transform2d>, Transform2d>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, GC, "Transform2d.__gc");
            lua_setfield(state, -2, "__gc");

            lua_pushcfunction(state, Index, "Transform2d.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Transform2d.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_pop(state, 1);

            lua_pushcfunction(state, New, name);
            lua_setglobal(state, name);
        }

        static Property m_properties[];
        static constexpr const char *name = "Transform2d";

    private:
        static int New(lua_State *state)
        {
            Push(state, new Transform2d(), true);
            return 1;
        }

        static int GC(lua_State *state)
        {
            return LuaBindingsBase<LuaBindings<Transform2d>, Transform2d>::GC(state);
        }
    };
} // namespace atmo::luau
