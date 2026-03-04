#include "body_2d.hpp"
#include <cmath>
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "box2d/box2d.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "core/resource/subresources/2d/shape/rectangle_shape2d.hpp"
#include "core/types.hpp"
#include "project/project_manager.hpp"
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
            .kind(flecs::PostUpdate)
            .each([](flecs::entity e, components::Transform2d &transform, Body2dData &body_data) {
                transform.position = b2Body_GetPosition(body_data.body_id);
                transform.rotation = atmo::common::math::RadiansToDegrees(b2Rot_GetAngle(b2Body_GetRotation(body_data.body_id)));
            });
    }

    void Body2d::initialize()
    {
        Entity2d::initialize();

        setComponent<Body2dData>({});
        auto body_data = p_handle.get_ref<Body2dData>();

        setBodyType();

        initBody();
    }

    void Body2d::initBody()
    {
        auto scene = getScene();

        if (!scene)
            return;

        auto body_data = p_handle.get_ref<Body2dData>();

        if (b2Body_IsValid(body_data->body_id))
            b2DestroyBody(body_data->body_id);

        auto transform = p_handle.get_ref<components::Transform2d>();

        body_data->body_def.position = transform->position;
        body_data->body_def.rotation = b2MakeRot(atmo::common::math::DegreesToRadians(transform->rotation));

        body_data->body_id = b2CreateBody(scene->getWorldId(), &body_data->body_def);

        // Shapes may be added before the body exists (e.g. before parent/scene is set).
        // Recreate all stored shapes on the newly created body.
        for (auto &shape : body_data->shapes) {
            if (shape) {
                shape->create(body_data->body_id);
            }
        }
    }

    void Body2d::setPosition(const types::Vector2 &position)
    {
        Entity2d::setPosition(position);

        auto transform = p_handle.get_ref<components::Transform2d>();

        auto body_data = p_handle.get_ref<Body2dData>();

        if (!b2Body_IsValid(body_data->body_id))
            return;

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

        if (!b2Body_IsValid(body_data->body_id))
            return;

        b2Body_SetTransform(body_data->body_id, transform->position, b2MakeRot(atmo::common::math::DegreesToRadians(rotation)));
    }

    void Body2d::setParent(Entity parent)
    {
        Entity2d::setParent(parent);
        initBody();
    }

    void Body2d::addShape(std::shared_ptr<resource::resources::Shape2d> shape)
    {
        auto body_data = p_handle.get_ref<Body2dData>();
        body_data->shapes.push_back(shape);

        if (!b2Body_IsValid(body_data->body_id))
            return;

        shape->create(body_data->body_id);
    }

    void Body2d::DebugRenderRectangleShape(SDL_Renderer *renderer, types::Vector2 center, types::Vector2 size, float angle)
    {
        static constexpr SDL_FColor outlineColor = { 0.18f, 0.93f, 1.0f, 1.0f };
        static constexpr SDL_FColor insideColor = { 0.18f, 0.93f, 1.0f, 0.25f };

        float rad = angle * (float)M_PI / 180.0f;
        float cosA = std::cosf(rad);
        float sinA = std::sinf(rad);

        float hw = size.x / 2.0f;
        float hh = size.y / 2.0f;

        SDL_FPoint local[4] = { { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh } };

        SDL_Vertex fillVerts[4];
        SDL_FPoint worldPoints[4];

        for (int i = 0; i < 4; i++) {
            float x = local[i].x;
            float y = local[i].y;

            float rx = x * cosA - y * sinA;
            float ry = x * sinA + y * cosA;

            worldPoints[i].x = center.x + rx;
            worldPoints[i].y = center.y + ry;

            fillVerts[i].position = worldPoints[i];
            fillVerts[i].color = insideColor;
            fillVerts[i].tex_coord = { 0, 0 };
        }

        // ---- Filled rectangle ----
        int indices[6] = { 0, 1, 2, 2, 3, 0 };
        SDL_RenderGeometry(renderer, nullptr, fillVerts, 4, indices, 6);

        // ---- Outline ----
        SDL_SetRenderDrawColor(renderer, outlineColor.r * 255, outlineColor.g * 255, outlineColor.b * 255, outlineColor.a * 255);
        SDL_RenderLines(renderer, worldPoints, 4);
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Body2d);
