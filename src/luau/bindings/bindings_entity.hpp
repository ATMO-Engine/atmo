#pragma once
#include <flecs.h>
#include <memory>
#include "lua_bindings.hpp"
#include "lualib.h"


#include "core/ecs/entities/2d/entity_2d.hpp"


#include <iostream>

namespace atmo::luau
{
    template <> class LuaBindings<flecs::entity> : public LuaBindingsBase<LuaBindings<flecs::entity>, flecs::entity>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            // __gc : détruit le shared_ptr quand Lua GC le userdata
            lua_pushcfunction(state, GC, "Entity.__gc");
            lua_setfield(state, -2, "__gc");

            // __index : accès aux propriétés et méthodes
            lua_pushcfunction(state, Index, "Entity.__index");
            lua_setfield(state, -2, "__index");

            // __newindex : écriture sur les propriétés
            lua_pushcfunction(state, NewIndex, "Entity.__newindex");
            lua_setfield(state, -2, "__newindex");

            // Table des méthodes accessibles depuis Luau
            lua_newtable(state);

            lua_pushcfunction(state, GetTransform, "Entity.getTransform");
            lua_setfield(state, -2, "getTransform");

            // Ajoute ici tes futures méthodes :
            // lua_pushcfunction(state, GetRigidbody, "Entity.getRigidbody");
            // lua_setfield(state, -2, "getRigidbody");

            lua_setfield(state, -2, "__methods");

            lua_pop(state, 1);
        }

        // Pas de m_properties pour l'instant — l'entité n'a pas de champs directs
        static Property m_properties[];
        static constexpr const char *name = "Entity";

    private:
        static int GC(lua_State *state);
        static int GetTransform(lua_State *state);
    };
} // namespace atmo::luau
