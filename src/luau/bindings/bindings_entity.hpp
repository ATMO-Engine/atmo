#pragma once
#include <flecs.h>
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "lua_bindings.hpp"
#include "lualib.h"

namespace atmo::luau
{
    template <> class LuaBindings<flecs::entity> : public LuaBindingsBase<LuaBindings<flecs::entity>, flecs::entity>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, GC, "Entity.__gc");
            lua_setfield(state, -2, "__gc");

            lua_pushcfunction(state, Index, "Entity.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Entity.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_pushcfunction(state, GetTransform, "Entity.getTransform");
            lua_setfield(state, -2, "getTransform");

            lua_pushcfunction(state, Name, "Entity.name");
            lua_setfield(state, -2, "name");

            lua_pushcfunction(state, GetChild, "Entity.getChild");
            lua_setfield(state, -2, "getChild");

            lua_pushcfunction(state, GetParent, "Entity.getParent");
            lua_setfield(state, -2, "getParent");

            lua_pushcfunction(state, IsAlive, "Entity.isAlive");
            lua_setfield(state, -2, "isAlive");

            lua_setfield(state, -2, "__methods");

            lua_pop(state, 1);
        }

        static Property m_properties[];
        static constexpr const char *name = "Entity";

    private:
        static int GC(lua_State *state);
        static int GetTransform(lua_State *state);
        static int Name(lua_State *state);
        static int GetChild(lua_State *state);
        static int GetParent(lua_State *state);
        static int IsAlive(lua_State *state);
    };
} // namespace atmo::luau
