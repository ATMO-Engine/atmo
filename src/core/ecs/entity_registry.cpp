#include "entity_registry.hpp"
#include "core/ecs/entities/entity.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs
{
    void EntityRegistry::SetWorld(flecs::world *world)
    {
        auto &instance = Instance();

        if (instance.m_world != nullptr) {
            UnregisterAll(world);
        }

        instance.m_world = world;

        for (auto &reg : instance.m_registers) {
            reg.components(world);
            reg.systems(world);
        }
    }

    void EntityRegistry::UnregisterAll(flecs::world *world)
    {
        auto &instance = Instance();

        for (auto &reg : instance.m_registers) {
            reg.unregister(world);
        }
    }
} // namespace atmo::core::ecs
