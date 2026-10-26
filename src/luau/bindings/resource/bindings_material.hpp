#pragma once

#include <memory>

#include "core/resource/material.hpp"
#include "lualib.h"
#include "luau/bindings/lua_bindings.hpp"
#include "luau/bindings/value/value_binding_base.hpp"

namespace atmo::luau
{
    template <>
    class LuaBindings<std::shared_ptr<core::resource::Material>>
        : public ValueLuaBindings<LuaBindings<std::shared_ptr<core::resource::Material>>, std::shared_ptr<core::resource::Material>>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, Index, "Material.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Material.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_pushcfunction(state, Set, "Material.set");
            lua_setfield(state, -2, "set");

            lua_pushcfunction(state, Get, "Material.get");
            lua_setfield(state, -2, "get");

            lua_pushcfunction(state, GetShader, "Material.getShader");
            lua_setfield(state, -2, "getShader");

            lua_setfield(state, -2, "__methods");

            RegisterProperties(state);

            Seal(state);

            lua_pop(state, 1);

            spdlog::debug("Material bindings");
        }

        static Property m_properties[];
        static constexpr const char *name = "Material";

    private:
        static int Set(lua_State *state);
        static int Get(lua_State *state);
        static int GetShader(lua_State *state);
    };
} // namespace atmo::luau
