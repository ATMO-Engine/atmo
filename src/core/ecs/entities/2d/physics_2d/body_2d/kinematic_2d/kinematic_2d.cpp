#include "kinematic_2d.hpp"
#include "box2d/box2d.h"
#include "common/math.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void Kinematic2d::RegisterSystems(flecs::world *world)
    {
        world->system<components::Transform2d, Body2dData, Kinematic2dData>("Kinematic2d_UpdateValuesFromPhysicsEngine")
            .kind(flecs::PostUpdate)
            .each([](flecs::entity e, components::Transform2d &t, Body2dData &bd, Kinematic2dData &kd) {
                if (!b2Body_IsValid(bd.body_id))
                    return;

                kd.linear_velocity = b2Body_GetLinearVelocity(bd.body_id);
                kd.angular_velocity = atmo::common::math::RadiansToDegrees(b2Body_GetAngularVelocity(bd.body_id));
            });

        world->system<Body2dData, Kinematic2dData>("Kinematic2d_PushVelocityToPhysicsEngine")
            .kind(flecs::OnValidate)
            .each([](flecs::entity e, Body2dData &bd, Kinematic2dData &kd) {
                if (!b2Body_IsValid(bd.body_id))
                    return;

                if (kd.linear_velocity != kd.synced_linear_velocity) {
                    b2Body_SetLinearVelocity(bd.body_id, kd.linear_velocity);
                    kd.synced_linear_velocity = kd.linear_velocity;
                }

                if (kd.angular_velocity != kd.synced_angular_velocity) {
                    b2Body_SetAngularVelocity(bd.body_id, common::math::RadiansToDegrees(kd.angular_velocity));
                    kd.synced_angular_velocity = kd.angular_velocity;
                }
            });
    }

    void Kinematic2d::initialize()
    {
        Body2d::initialize();

        setComponent<Kinematic2dData>({});
    }

    void Kinematic2d::setBodyType()
    {
        auto body_data = p_handle.get_ref<Body2dData>();
        body_data->body_def.type = b2_kinematicBody;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Kinematic2d);
ATMO_REGISTER_COMPONENT(atmo::core::ecs::entities::Kinematic2d::Kinematic2dData)
