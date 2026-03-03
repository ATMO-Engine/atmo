#include "body_2d.hpp"
#include "box2d/box2d.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void Body2d::RegisterComponents(flecs::world *world)
    {
        world->component<Body2dData>();
    }

    void Body2d::RegisterSystems(flecs::world *world)
    {
        world->system<components::Transform2d, Body2dData>("Body2d_UpdateValuesFromPhysicsEngine")
            .kind(flecs::OnStore)
            .each([](flecs::entity e, components::Transform2d &transform, Body2dData &body_data) {
                transform.position = b2Body_GetPosition(body_data.body_id);
                transform.rotation = atmo::common::math::RadiansToDegrees(b2Rot_GetAngle(b2Body_GetRotation(body_data.body_id)));
            });
    }

    void Body2d::initialize()
    {
        Entity2d::initialize();
        auto transform = p_handle.get_ref<components::Transform2d>();

        setComponent<Body2dData>({});
        auto body_data = p_handle.get_ref<Body2dData>();

        body_data->body_def.position = transform->position;
        body_data->body_def.rotation = b2MakeRot(atmo::common::math::DegreesToRadians(transform->rotation));
        body_data->body_id = b2CreateBody(getScene()->getWorldId(), &body_data->body_def);
    }

    void Body2d::setPosition(const types::Vector2 &position)
    {
        Entity2d::setPosition(position);

        auto transform = p_handle.get_ref<components::Transform2d>();

        auto body_data = p_handle.get_ref<Body2dData>();
        b2Body_SetTransform(body_data->body_id, position, b2MakeRot(atmo::common::math::DegreesToRadians(transform->rotation)));
    }

    void Body2d::setScale(const types::Vector2 &scale)
    {
        Entity2d::setScale(scale);

        auto body_data = p_handle.get_ref<Body2dData>();
        for (auto shape : body_data->shapes) {
            // TODO: implement shape scaling and apply it (recalculated global scale) here
            // shape->setScale(scale);
        }
    }

    void Body2d::setRotation(float rotation)
    {
        Entity2d::setRotation(rotation);

        auto transform = p_handle.get_ref<components::Transform2d>();

        auto body_data = p_handle.get_ref<Body2dData>();
        b2Body_SetTransform(body_data->body_id, transform->position, b2MakeRot(atmo::common::math::DegreesToRadians(rotation)));
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Body2d);
