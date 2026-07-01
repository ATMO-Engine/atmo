#include "bindings_entity.hpp"
#include <spdlog/spdlog.h>
#include "bindings_transform2.hpp"
#include "core/ecs/entities/2d/entity_2d.hpp"
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
} // namespace atmo::luau
