#include "bindings_entity.hpp"
#include <spdlog/spdlog.h>
#include "bindings_transform2.hpp"
#include "bindings_vector2.hpp"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/kinematic_2d/kinematic_2d.hpp"
#include "lua_bindings.hpp"

namespace atmo::luau
{
    Property LuaBindings<flecs::entity>::m_properties[] = { { nullptr, nullptr, nullptr } };

    int LuaBindings<flecs::entity>::GC(lua_State *state)
    {
        return LuaBindingsBase<LuaBindings<flecs::entity>, flecs::entity>::GC(state);
    }

    int LuaBindings<flecs::entity>::GetTransform(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        if (!entity->is_alive()) {
            lua_pushnil(state);
            spdlog::warn("Entity is not alive");
            return 1;
        }
        if (!entity->has<atmo::core::components::Transform2d>()) {
            lua_pushnil(state);
            spdlog::warn("No component Transform2d is the entity");
            return 1;
        }

        atmo::core::components::Transform2d &t = entity->get_mut<atmo::core::components::Transform2d>();

        LuaBindings<atmo::core::components::Transform2d>::Push(state, &t);
        return 1;
    }

    int LuaBindings<flecs::entity>::Name(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        if (!entity->is_alive()) {
            lua_pushnil(state);
            spdlog::warn("Entity is not alive");
            return 1;
        }

        lua_pushstring(state, entity->name().c_str());
        return 1;
    }

    int LuaBindings<flecs::entity>::GetChild(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        const char *childName = luaL_checkstring(state, 2);

        if (!entity) {
            return 0;
        }

        if (!entity->is_alive()) {
            lua_pushnil(state);
            spdlog::warn("Entity is not alive");
            return 1;
        }

        flecs::entity child = entity->lookup(childName);
        if (!child.is_valid() || !child.is_alive()) {
            lua_pushnil(state);
            return 1;
        }

        LuaBindings<flecs::entity>::Push(state, new flecs::entity(child), true);
        return 1;
    }

    int LuaBindings<flecs::entity>::GetParent(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        if (!entity->is_alive()) {
            lua_pushnil(state);
            spdlog::warn("Entity is not alive");
            return 1;
        }

        flecs::entity parent = entity->parent();
        if (!parent.is_valid() || !parent.is_alive()) {
            lua_pushnil(state);
            return 1;
        }

        LuaBindings<flecs::entity>::Push(state, new flecs::entity(parent), true);
        return 1;
    }

    int LuaBindings<flecs::entity>::IsAlive(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        if (!entity) {
            return 0;
        }

        lua_pushboolean(state, entity->is_alive());
        return 1;
    }

    int LuaBindings<flecs::entity>::ApplyLinearVelocity(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        auto *vec = LuaBindings<atmo::core::types::Vector2>::CheckPtr(state, 2);

        if (!entity || !vec) {
            return 0;
        }

        if (!entity->is_alive()) {
            spdlog::warn("Entity is not alive");
            return 0;
        }

        if (!entity->has<core::ecs::entities::Kinematic2d::Kinematic2dData>()) {
            luaL_error(state, "applyLinearVelocity: entity has no Kinematic2dData component");
        }

        core::ecs::entities::Kinematic2d::Kinematic2dData &k = entity->get_mut<core::ecs::entities::Kinematic2d::Kinematic2dData>();
        k.linear_velocity = *vec;

        return 0;
    }

    int LuaBindings<flecs::entity>::ApplyAngularVelocity(lua_State *state)
    {
        auto *entity = CheckPtr(state, 1);
        float value = (float)luaL_checknumber(state, 2);

        if (!entity) {
            return 0;
        }

        if (!entity->is_alive()) {
            spdlog::warn("Entity is not alive");
            return 0;
        }

        if (!entity->has<core::ecs::entities::Kinematic2d::Kinematic2dData>()) {
            luaL_error(state, "applyAngularVelocity: entity has no Kinematic2dData component");
        }

        core::ecs::entities::Kinematic2d::Kinematic2dData &k = entity->get_mut<core::ecs::entities::Kinematic2d::Kinematic2dData>();
        k.angular_velocity = value;

        return 0;
    }
} // namespace atmo::luau
