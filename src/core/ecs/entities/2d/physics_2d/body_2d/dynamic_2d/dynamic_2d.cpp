#include "dynamic_2d.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void Dynamic2d::RegisterSystems(flecs::world *world) {}

    void Dynamic2d::initialize()
    {
        Body2d::initialize();
    }

    void Dynamic2d::setBodyType()
    {
        auto body_data = p_handle.get_ref<Body2dData>();
        body_data->body_def.type = b2_dynamicBody;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Dynamic2d);
