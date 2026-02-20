#include "entity_2d.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    void Entity2d::RegisterComponents(flecs::world *world)
    {
        world->component<components::Transform2d>();
    }

    void Entity2d::RegisterSystems(flecs::world *world)
    {
        world->system<components::Transform2d, components::Transform2d>("Transform2d_GenerateGlobal")
            .kind(flecs::PreUpdate)
            .term_at(1)
            .up()
            .each([](flecs::entity e, components::Transform2d &t, const components::Transform2d &parent_t) {
                t.g_position = { parent_t.g_position.x + t.position.x, parent_t.g_position.y + t.position.y };
                t.g_rotation = parent_t.g_rotation + t.rotation;
                t.g_scale = { parent_t.g_scale.x * t.scale.x, parent_t.g_scale.y * t.scale.y };
            });
    }

    void Entity2d::initialize()
    {
        Entity::initialize();

        setComponent<components::Transform2d>({});
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Entity2d);
