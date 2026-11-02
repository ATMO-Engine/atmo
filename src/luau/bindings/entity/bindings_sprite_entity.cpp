#include "bindings_sprite_entity.hpp"

#include <spdlog/spdlog.h>

#include "luau/bindings/EntityLuauRegistry.hpp"
#include "luau/bindings/resource/bindings_material.hpp"
#include "luau/bindings/value/bindings_color.hpp"

namespace atmo::luau
{
    namespace
    {
        core::ecs::entities::Sprite2d *AliveSprite(lua_State *state)
        {
            auto *entity = LuaBindings<core::ecs::entities::Sprite2d>::CheckPtr(state, 1);
            if (!entity || !entity->getHandle().is_alive() || !entity->getHandle().has<core::components::Sprite2d>()) {
                spdlog::warn("Sprite2d is not alive");
                return nullptr;
            }
            return entity;
        }
    } // namespace

    Property LuaBindings<core::ecs::entities::Sprite2d>::m_properties[] = { { nullptr, nullptr, nullptr } };

    int LuaBindings<core::ecs::entities::Sprite2d>::GetMaterial(lua_State *state)
    {
        auto *sprite = AliveSprite(state);
        std::shared_ptr<core::resource::Material> material = sprite ? sprite->getMaterial() : nullptr;
        if (!material)
            lua_pushnil(state);
        else
            LuaBindings<std::shared_ptr<core::resource::Material>>::Push(state, material);
        return 1;
    }

    int LuaBindings<core::ecs::entities::Sprite2d>::SetMaterial(lua_State *state)
    {
        const char *path = luaL_optstring(state, 2, "");
        if (auto *sprite = AliveSprite(state))
            sprite->setMaterialPath(path);
        return 0;
    }

    int LuaBindings<core::ecs::entities::Sprite2d>::GetModulate(lua_State *state)
    {
        auto *sprite = AliveSprite(state);
        LuaBindings<core::types::Color>::Push(state, sprite ? sprite->getModulate() : core::types::Color::WHITE);
        return 1;
    }

    int LuaBindings<core::ecs::entities::Sprite2d>::SetModulate(lua_State *state)
    {
        const auto *color = LuaBindings<core::types::Color>::CheckPtr(state, 2);
        if (auto *sprite = AliveSprite(state))
            sprite->setModulate(*color);
        return 0;
    }

    int LuaBindings<core::ecs::entities::Sprite2d>::GetCustom(lua_State *state)
    {
        auto *sprite = AliveSprite(state);
        const core::types::Vector4 custom = sprite ? sprite->getCustom() : core::types::Vector4{};
        lua_pushnumber(state, custom.x);
        lua_pushnumber(state, custom.y);
        lua_pushnumber(state, custom.z);
        lua_pushnumber(state, custom.w);
        return 4;
    }

    int LuaBindings<core::ecs::entities::Sprite2d>::SetCustom(lua_State *state)
    {
        const core::types::Vector4 custom{ static_cast<float>(luaL_checknumber(state, 2)),
                                           static_cast<float>(luaL_optnumber(state, 3, 0.0)),
                                           static_cast<float>(luaL_optnumber(state, 4, 0.0)),
                                           static_cast<float>(luaL_optnumber(state, 5, 0.0)) };
        if (auto *sprite = AliveSprite(state))
            sprite->setCustom(custom);
        return 0;
    }
} // namespace atmo::luau

ATMO_REGISTER_LUA_ENTITY_BINDING(atmo::core::ecs::entities::Sprite2d);
