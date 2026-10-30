#pragma once
#include <flecs.h>
#include "core/ecs/entities/2d/sprite_2d/sprite_2d.hpp"
#include "lualib.h"
#include "luau/bindings/entity/bindings_entity.hpp"
#include "luau/bindings/lua_bindings.hpp"

namespace atmo::luau
{
    template <>
    class LuaBindings<core::ecs::entities::Sprite2d>
        : public EntityLuaBindings<LuaBindings<core::ecs::entities::Sprite2d>, core::ecs::entities::Sprite2d, LuaBindings<core::ecs::entities::Entity>>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, Index, "Sprite2d.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Sprite2d.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_pushcfunction(state, GetMaterial, "Sprite2d.getMaterial");
            lua_setfield(state, -2, "getMaterial");

            lua_pushcfunction(state, SetMaterial, "Sprite2d.setMaterial");
            lua_setfield(state, -2, "setMaterial");

            lua_pushcfunction(state, GetModulate, "Sprite2d.getModulate");
            lua_setfield(state, -2, "getModulate");

            lua_pushcfunction(state, SetModulate, "Sprite2d.setModulate");
            lua_setfield(state, -2, "setModulate");

            lua_pushcfunction(state, GetCustom, "Sprite2d.getCustom");
            lua_setfield(state, -2, "getCustom");

            lua_pushcfunction(state, SetCustom, "Sprite2d.setCustom");
            lua_setfield(state, -2, "setCustom");

            ChainTable(state, LuaBindings<atmo::core::ecs::entities::Entity>::name, "__methods");

            lua_setfield(state, -2, "__methods");

            RegisterProperties(state);

            RegisterIsA(state);

            Seal(state);

            lua_pop(state, 1);

            spdlog::debug("Sprite2d bindings");
        }

        static Property m_properties[];
        static constexpr const char *name = "Sprite2d";

    private:
        static int GetMaterial(lua_State *state);
        static int SetMaterial(lua_State *state);
        static int GetModulate(lua_State *state);
        static int SetModulate(lua_State *state);
        static int GetCustom(lua_State *state);
        static int SetCustom(lua_State *state);
    };
} // namespace atmo::luau
