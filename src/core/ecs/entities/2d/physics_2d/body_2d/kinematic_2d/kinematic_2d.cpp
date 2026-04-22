#include "kinematic_2d.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void Kinematic2d::RegisterSystems(flecs::world *world) {}

    void Kinematic2d::initialize()
    {
        Body2d::initialize();

        addTag<Kinematic2dData>();
    }

    void Kinematic2d::setBodyType()
    {
        auto body_data = p_handle.get_ref<Body2dData>();
        body_data->body_def.type = b2_kinematicBody;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Kinematic2d);
ATMO_REGISTER_COMPONENT(atmo::core::ecs::entities::Kinematic2d::Kinematic2dData)
