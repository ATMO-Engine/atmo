#pragma once

#include "core/ecs/entities/2d/entity_2d.hpp"
#include "lualib.h"
#include "luau/bindings/component/component_binding_base.hpp"
#include "luau/bindings/lua_bindings.hpp"

namespace atmo::luau
{
    using namespace atmo::core::components;

    template <> class LuaBindings<Transform2d> : public ComponentLuaBindings<LuaBindings<Transform2d>, Transform2d, void>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, Index, "Transform2d.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Transform2d.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);
            lua_setfield(state, -2, "__methods");

            RegisterProperties(state);

            Seal(state);

            lua_pop(state, 1);

            lua_pushcfunction(state, New, name);
            lua_setglobal(state, name);

            spdlog::debug("Transform bindings");
        }

        static Property m_properties[];
        static constexpr const char *name = "Transform2d";

    private:
        static int New(lua_State *state)
        {
            PushOwned(state, new Transform2d());
            return 1;
        }
    };
} // namespace atmo::luau
