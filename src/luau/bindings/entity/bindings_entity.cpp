#include "bindings_entity.hpp"
#include <spdlog/spdlog.h>
#include "core/ecs/entities/entity.hpp"
#include "flecs/addons/cpp/entity.hpp"
#include "luau/bindings/component/bindings_transform2.hpp"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "luau/bindings/lua_bindings.hpp"
#include "luau/bindings/EntityLuauRegistry.hpp"

namespace atmo::luau
{
    Property LuaBindings<core::ecs::entities::Entity>::m_properties[] = { { nullptr, nullptr, nullptr } };

    int LuaBindings<core::ecs::entities::Entity>::GetTransform(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            lua_pushnil(state);
            spdlog::warn("Entity is not alive");
            return 1;
        }
        if (!handle.has<atmo::core::components::Transform2d>()) {
            lua_pushnil(state);
            spdlog::warn("No component Transform2d is the entity");
            return 1;
        }

        LuaBindings<atmo::core::components::Transform2d>::PushComponent(state, handle);
        return 1;
    }

    int LuaBindings<core::ecs::entities::Entity>::Name(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            lua_pushnil(state);
            spdlog::warn("Entity is not alive");
            return 1;
        }

        lua_pushstring(state, handle.name().c_str());
        return 1;
    }

    int LuaBindings<core::ecs::entities::Entity>::GetChild(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        const char *childName = luaL_checkstring(state, 2);

        if (!entity) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            lua_pushnil(state);
            spdlog::warn("Entity is not alive");
            return 1;
        }

        flecs::entity child = handle.lookup(childName);
        if (!child.is_alive()) {
            lua_pushnil(state);
            return 1;
        }

        if (!LuauRegistry::Instance().pushEntity(state, child)) {
            lua_pushnil(state);
        }
        return 1;
    }

    int LuaBindings<core::ecs::entities::Entity>::GetParent(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            lua_pushnil(state);
            spdlog::warn("Entity is not alive");
            return 1;
        }

        flecs::entity parent = handle.parent();
        if (!parent.is_alive()) {
            lua_pushnil(state);
            return 1;
        }

        if (!LuauRegistry::Instance().pushEntity(state, parent)) {
            lua_pushnil(state);
        }
        return 1;
    }

    int LuaBindings<core::ecs::entities::Entity>::IsAlive(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        auto handle = entity->getHandle();
        lua_pushboolean(state, handle.is_alive());
        return 1;
    }
} // namespace atmo::luau

ATMO_REGISTER_LUA_ENTITY_BINDING(atmo::core::ecs::entities::Entity);
