#include "entity_2d.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    static void RegisterSystems(flecs::world *world)
    {
        world->component<components::Transform2d>();
    }

    void Entity2d::initialize()
    {
        Entity::initialize();
        setComponent<components::Transform2d>({});
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Entity2d);
