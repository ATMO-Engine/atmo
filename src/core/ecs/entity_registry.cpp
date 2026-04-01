#include "entity_registry.hpp"
#include "core/ecs/entities/entity.hpp"
#include "spdlog/spdlog.h"
#include <memory>

namespace atmo::core::ecs
{
    void EntityRegistry::SetWorld(flecs::world *world)
    {
        auto &instance = Instance();

        if (instance.m_world != nullptr) {
            UnregisterAll(world);
        }

        instance.m_world = world;

        for (auto &reg : instance.m_registers) reg.systems(world);
    }

    std::unique_ptr<entities::Entity> EntityRegistry::Wrap(const entities::Entity &e)
    {
        if (!e.hasComponent<components::EntityType>())
            return nullptr;

        const auto &type_name = e.getComponent<components::EntityType>().type_name;
        auto it = Instance().m_wrap_factories.find(type_name);
        if (it == Instance().m_wrap_factories.end())
            return nullptr;

        return std::unique_ptr<entities::Entity>(it->second(static_cast<flecs::entity>(e)));
    }

    void EntityRegistry::UnregisterAll(flecs::world *world)
    {
        auto &instance = Instance();

        for (auto &reg : instance.m_registers) {
            reg.unregister(world);
        }
    }
} // namespace atmo::core::ecs
