#include "static_2d.hpp"

namespace atmo::core::ecs::entities
{
    void Static2d::RegisterComponents(flecs::world *world)
    {
        world->component<Static2dData>();
    }

    void Static2d::RegisterSystems(flecs::world *world) {}

    void Static2d::initialize()
    {
        Body2d::initialize();
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Static2d);
