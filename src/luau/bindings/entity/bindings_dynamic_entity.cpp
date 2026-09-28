#include "bindings_dynamic_entity.hpp"
#include <spdlog/spdlog.h>
#include "flecs/addons/cpp/entity.hpp"
#include "luau/bindings/value/bindings_vector2.hpp"
#include "box2d/box2d.h"
#include "core/ecs/entities/2d/physics_2d/body_2d/dynamic_2d/dynamic_2d.hpp"
#include "luau/bindings/lua_bindings.hpp"
#include "luau/bindings/EntityLuauRegistry.hpp"

namespace atmo::luau
{
    Property LuaBindings<core::ecs::entities::Dynamic2d>::m_properties[] = { { nullptr, nullptr, nullptr } };

    int LuaBindings<core::ecs::entities::Dynamic2d>::ApplyLinearVelocity(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        auto *vec = LuaBindings<atmo::core::types::Vector2>::CheckPtr(state, 2);

        if (!entity || !vec) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            spdlog::warn("Entity is not alive");
            return 0;
        }

        if (handle.has<core::ecs::entities::Dynamic2d::Dynamic2dData>()) {
            handle.get_mut<core::ecs::entities::Dynamic2d::Dynamic2dData>().linear_velocity = *vec;
        } else {
            luaL_error(state, "applyLinearVelocity: entity has no Dynamic2dData component");
        }

        return 0;
    }

    int LuaBindings<core::ecs::entities::Dynamic2d>::ApplyAngularVelocity(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        float value = (float)luaL_checknumber(state, 2);

        if (!entity) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            spdlog::warn("Entity is not alive");
            return 0;
        }

        if (handle.has<core::ecs::entities::Dynamic2d::Dynamic2dData>()) {
            handle.get_mut<core::ecs::entities::Dynamic2d::Dynamic2dData>().angular_velocity = value;
        } else {
            luaL_error(state, "applyAngularVelocity: entity has no Kinematic2dData or Dynamic2dData component");
        }

        return 0;
    }

    int LuaBindings<core::ecs::entities::Dynamic2d>::ApplyLinearImpulse(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        auto *vec = LuaBindings<atmo::core::types::Vector2>::CheckPtr(state, 2);
        bool wake = lua_gettop(state) >= 3 ? (bool)lua_toboolean(state, 3) : true;

        if (!entity || !vec) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            spdlog::warn("Entity is not alive");
            return 0;
        }

        if (!handle.has<core::ecs::entities::Body2d::Body2dData>()) {
            luaL_error(state, "applyLinearImpulse: entity '%s' has no Body2dData component", handle.name().c_str());
        }

        core::ecs::entities::Body2d::Body2dData &bd = handle.get_mut<core::ecs::entities::Body2d::Body2dData>();
        if (!b2Body_IsValid(bd.body_id)) {
            return 0;
        }

        b2Body_ApplyLinearImpulseToCenter(bd.body_id, *vec, wake);

        return 0;
    }

    int LuaBindings<core::ecs::entities::Dynamic2d>::ApplyAngularImpulse(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        float value = (float)luaL_checknumber(state, 2);
        bool wake = lua_gettop(state) >= 3 ? (bool)lua_toboolean(state, 3) : true;

        if (!entity) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            spdlog::warn("Entity is not alive");
            return 0;
        }

        if (!handle.has<core::ecs::entities::Body2d::Body2dData>()) {
            luaL_error(state, "applyAngularImpulse: entity has no Body2dData component");
        }

        core::ecs::entities::Body2d::Body2dData &bd = handle.get_mut<core::ecs::entities::Body2d::Body2dData>();
        if (!b2Body_IsValid(bd.body_id)) {
            return 0;
        }

        b2Body_ApplyAngularImpulse(bd.body_id, value, wake);

        return 0;
    }

    int LuaBindings<core::ecs::entities::Dynamic2d>::GetLinearVelocity(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            lua_pushnil(state);
            return 1;
        }

        if (handle.has<core::ecs::entities::Dynamic2d::Dynamic2dData>()) {
            core::ecs::entities::Dynamic2d::Dynamic2dData &d = handle.get_mut<core::ecs::entities::Dynamic2d::Dynamic2dData>();
            LuaBindings<atmo::core::types::Vector2>::Push(state, d.linear_velocity);
        } else {
            lua_pushnil(state);
        }
        return 1;
    }

    int LuaBindings<core::ecs::entities::Dynamic2d>::GetAngularVelocity(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        auto handle = entity->getHandle();
        if (!handle.is_alive()) {
            lua_pushnil(state);
            return 1;
        }

        if (handle.has<core::ecs::entities::Dynamic2d::Dynamic2dData>()) {
            lua_pushnumber(state, handle.get_mut<core::ecs::entities::Dynamic2d::Dynamic2dData>().angular_velocity);
        } else {
            lua_pushnil(state);
        }
        return 1;
    }
} // namespace atmo::luau

ATMO_REGISTER_LUA_ENTITY_BINDING(atmo::core::ecs::entities::Dynamic2d);
