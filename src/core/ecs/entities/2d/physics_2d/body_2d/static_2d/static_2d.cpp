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

        addTag<Static2dData>();
    }

    void Static2d::setBodyType()
    {
        auto body_data = p_handle.get_ref<Body2dData>();
        body_data->body_def.type = b2_staticBody;
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Static2d);
