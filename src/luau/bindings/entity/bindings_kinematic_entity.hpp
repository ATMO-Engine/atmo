#pragma once
#include <flecs.h>
#include "core/ecs/entities/2d/physics_2d/body_2d/kinematic_2d/kinematic_2d.hpp"
#include "core/ecs/entities/entity.hpp"
#include "luau/bindings/entity/bindings_entity.hpp"
#include "luau/bindings/lua_bindings.hpp"
#include "lualib.h"

namespace atmo::luau
{
    template <> class LuaBindings<core::ecs::entities::Kinematic2d> : public EntityLuaBindings<LuaBindings<core::ecs::entities::Kinematic2d>, core::ecs::entities::Kinematic2d, LuaBindings<core::ecs::entities::Entity>>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, Index, "Kinematic2d.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Kinematic2d.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_pushcfunction(state, ApplyLinearVelocity, "Kinematic2d.applyLinearVelocity");
            lua_setfield(state, -2, "applyLinearVelocity");

            lua_pushcfunction(state, ApplyAngularVelocity, "Kinematic2d.applyAngularVelocity");
            lua_setfield(state, -2, "applyAngularVelocity");

            lua_pushcfunction(state, ApplyLinearImpulse, "Kinematic2d.applyLinearImpulse");
            lua_setfield(state, -2, "applyLinearImpulse");

            lua_pushcfunction(state, ApplyAngularImpulse, "Kinematic2d.applyAngularImpulse");
            lua_setfield(state, -2, "applyAngularImpulse");

            lua_pushcfunction(state, GetLinearVelocity, "Kinematic2d.getLinearVelocity");
            lua_setfield(state, -2, "getLinearVelocity");

            lua_pushcfunction(state, GetAngularVelocity, "Kinematic2d.getAngularVelocity");
            lua_setfield(state, -2, "getAngularVelocity");

            ChainTable(state, LuaBindings<atmo::core::ecs::entities::Entity>::name, "__methods");

            lua_setfield(state, -2, "__methods");

            RegisterProperties(state);

            RegisterIsA(state);

            Seal(state);

            lua_pop(state, 1);

            spdlog::debug("Kinematic bindings");
        }

        static Property m_properties[];
        static constexpr const char *name = "Kinematic2d";

    private:
        static int ApplyLinearVelocity(lua_State *state);
        static int ApplyAngularVelocity(lua_State *state);
        static int ApplyLinearImpulse(lua_State *state);
        static int ApplyAngularImpulse(lua_State *state);
        static int GetLinearVelocity(lua_State *state);
        static int GetAngularVelocity(lua_State *state);
    };
} // namespace atmo::luau
