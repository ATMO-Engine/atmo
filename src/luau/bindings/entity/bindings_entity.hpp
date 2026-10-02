#pragma once
#include <flecs.h>
#include "core/ecs/entities/entity.hpp"
#include "entity_binding_base.hpp"
#include "lualib.h"
#include "luau/bindings/lua_bindings.hpp"

namespace atmo::luau
{
    template <>
    class LuaBindings<core::ecs::entities::Entity> : public EntityLuaBindings<LuaBindings<core::ecs::entities::Entity>, core::ecs::entities::Entity, void>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

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

            RegisterProperties(state);

            RegisterIsA(state);

            Seal(state);

            lua_pop(state, 1);

            spdlog::debug("Entity bindings");
        }

        static Property m_properties[];
        static constexpr const char *name = "Entity";

    protected:
        static int GetTransform(lua_State *state);
        static int Name(lua_State *state);
        static int GetChild(lua_State *state);
        static int GetParent(lua_State *state);
        static int IsAlive(lua_State *state);
    };
} // namespace atmo::luau
