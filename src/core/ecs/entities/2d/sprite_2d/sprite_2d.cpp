#include "sprite_2d.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void Sprite2d::RegisterComponents(flecs::world *world)
    {
        world->component<components::Sprite2d>();
    }

    void Sprite2d::RegisterSystems(flecs::world *world) {}

    void Sprite2d::initialize()
    {
        Entity::initialize();

        setComponent<components::Sprite2d>({});
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Sprite2d);
