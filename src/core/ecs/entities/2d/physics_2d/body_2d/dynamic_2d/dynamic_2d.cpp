#include "dynamic_2d.hpp"
#include "box2d/box2d.h"
#include "common/math.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void Dynamic2d::RegisterSystems(flecs::world *world)
    {
        world->system<Body2dData, Dynamic2dData>("Dynamic2d_UpdateValuesFromPhysicsEngine")
            .kind(flecs::PostUpdate)
            .each([](flecs::entity e, Body2dData &bd, Dynamic2dData &dd) {
                if (!b2Body_IsValid(bd.body_id))
                    return;

                dd.linear_velocity = b2Body_GetLinearVelocity(bd.body_id);
                dd.synced_linear_velocity = dd.linear_velocity;

                dd.angular_velocity = atmo::common::math::RadiansToDegrees(b2Body_GetAngularVelocity(bd.body_id));
                dd.synced_angular_velocity = dd.angular_velocity;
            });

        world->system<Body2dData, Dynamic2dData>("Dynamic2d_PushVelocityToPhysicsEngine")
            .kind(flecs::OnValidate)
            .each([](flecs::entity e, Body2dData &bd, Dynamic2dData &dd) {
                if (!b2Body_IsValid(bd.body_id))
                    return;

                if (dd.linear_velocity != dd.synced_linear_velocity) {
                    b2Body_SetLinearVelocity(bd.body_id, dd.linear_velocity);
                    dd.synced_linear_velocity = dd.linear_velocity;
                }

                if (dd.angular_velocity != dd.synced_angular_velocity) {
                    b2Body_SetAngularVelocity(bd.body_id, common::math::DegreesToRadians(dd.angular_velocity));
                    dd.synced_angular_velocity = dd.angular_velocity;
                }
            });
    }

    void Dynamic2d::initialize()
    {
        Body2d::initialize();

        setComponent<Dynamic2dData>({});
    }

    void Dynamic2d::setBodyType()
    {
        auto body_data = p_handle.get_ref<Body2dData>();
        body_data->body_def.type = b2_dynamicBody;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Dynamic2d);
ATMO_REGISTER_COMPONENT(atmo::core::ecs::entities::Dynamic2d::Dynamic2dData)
