#include "body_2d.hpp"
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

    void Body2d::RegisterSystems(flecs::world *world) {}

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
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Body2d);
