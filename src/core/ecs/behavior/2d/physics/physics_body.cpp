#include <functional>
#include <unordered_map>
#include "box2d/box2d.h"
#include "box2d/math_functions.h"
#include "common/math.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/ecs.hpp"
#include "core/ecs/ecs_registry.hpp"
#include "core/types.hpp"
#include "flecs.h"
#include "impl/window.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

static b2ShapeId createRectangleShape(flecs::entity &entity, atmo::core::components::PhysicsBody2d &physics_body, b2ShapeDef *shapeDef)
{
    const atmo::core::components::RectangleShape2d &rectangle_shape = entity.get<atmo::core::components::RectangleShape2d>();
    b2Polygon rectangle =
        b2MakeBox(atmo::common::math::PixelToMeter(rectangle_shape.size.x) / 2.0f, atmo::common::math::PixelToMeter(rectangle_shape.size.x) / 2.0f);

    return b2CreatePolygonShape(physics_body.body_id, shapeDef, &rectangle);
}

static b2ShapeId createCircleShape(flecs::entity &entity, atmo::core::components::PhysicsBody2d &physics_body, b2ShapeDef *shapeDef)
{
    const atmo::core::components::CircleShape2d &circle_shape = entity.get<atmo::core::components::CircleShape2d>();
    b2Circle circle{};

    circle.center = b2Vec2{ 0.0f, 0.0f };
    circle.radius = atmo::common::math::PixelToMeter(circle_shape.radius);

    return b2CreateCircleShape(physics_body.body_id, shapeDef, &circle);
}

static b2ShapeId createCapsuleShape(flecs::entity &entity, atmo::core::components::PhysicsBody2d &physics_body, b2ShapeDef *shapeDef)
{
    const atmo::core::components::CapsuleShape2d &capsule_shape = entity.get<atmo::core::components::CapsuleShape2d>();
    b2Capsule capsule{};

    capsule.center1 = b2Vec2{ 0.0f, -atmo::common::math::PixelToMeter(capsule_shape.height) / 2.0f + atmo::common::math::PixelToMeter(capsule_shape.radius) };
    capsule.center2 = b2Vec2{ 0.0f, atmo::common::math::PixelToMeter(capsule_shape.height) / 2.0f - atmo::common::math::PixelToMeter(capsule_shape.radius) };
    capsule.radius = atmo::common::math::PixelToMeter(capsule_shape.radius);

    return b2CreateCapsuleShape(physics_body.body_id, shapeDef, &capsule);
}

static b2ShapeId createPolygonShape(flecs::entity &entity, atmo::core::components::PhysicsBody2d &physics_body, b2ShapeDef *shapeDef)
{
    const atmo::core::components::PolygonShape2d &polygon_shape = entity.get<atmo::core::components::PolygonShape2d>();
    std::vector<b2Vec2> vertices;
    for (const auto &point : polygon_shape.points)
        vertices.push_back(b2Vec2{ atmo::common::math::PixelToMeter(point.x), atmo::common::math::PixelToMeter(point.y) });

    b2Hull hull = b2ComputeHull(vertices.data(), vertices.size());
    b2Polygon polygon = b2MakePolygon(&hull, 0.0f);

    return b2CreatePolygonShape(physics_body.body_id, shapeDef, &polygon);
}

static const std::unordered_map<
    atmo::core::types::Shape2dType, std::function<b2ShapeId(flecs::entity &, atmo::core::components::PhysicsBody2d &, b2ShapeDef *)>>
    shape_creators = { { atmo::core::types::Shape2dType::Rectangle, createRectangleShape },
                       { atmo::core::types::Shape2dType::Circle, createCircleShape },
                       { atmo::core::types::Shape2dType::Capsule, createCapsuleShape },
                       { atmo::core::types::Shape2dType::Polygon, createPolygonShape } };

static void createShapeForBody(flecs::entity &entity, b2BodyId bodyId, atmo::core::components::PhysicsBody2d &physics_body)
{
    if (physics_body.shape == atmo::core::types::Shape2dType::None)
        return;

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    b2ShapeId shapeId = shape_creators.at(physics_body.shape)(entity, physics_body, &shapeDef);
    spdlog::debug("Created shape {}", entity.world().to_expr(&physics_body.shape).c_str());
}

void setupPhysicsBody2dBehaviors(flecs::world world)
{
    using namespace atmo::core;

    world.observer<components::PhysicsBody2d>("Shape2d_ComponentGenerator")
        .event(flecs::OnSet)
        .each([](flecs::entity entity, components::PhysicsBody2d &physics_body) {
            switch (physics_body.shape) {
                case atmo::core::types::Shape2dType::Rectangle:
                    if (entity.has<components::RectangleShape2d>())
                        return;
                case atmo::core::types::Shape2dType::Circle:
                    if (entity.has<components::CircleShape2d>())
                        return;
                case atmo::core::types::Shape2dType::Capsule:
                    if (entity.has<components::CapsuleShape2d>())
                        return;
                case atmo::core::types::Shape2dType::Polygon:
                    if (entity.has<components::PolygonShape2d>())
                        return;
                case atmo::core::types::Shape2dType::None:
                    break;
            }

            entity.remove<components::RectangleShape2d>();
            entity.remove<components::CircleShape2d>();
            entity.remove<components::CapsuleShape2d>();
            entity.remove<components::PolygonShape2d>();

            switch (physics_body.shape) {
                case atmo::core::types::Shape2dType::Rectangle:
                    entity.set<components::RectangleShape2d>({});
                    break;
                case atmo::core::types::Shape2dType::Circle:
                    entity.set<components::CircleShape2d>({});
                    break;
                case atmo::core::types::Shape2dType::Capsule:
                    entity.set<components::CapsuleShape2d>({});
                    break;
                case atmo::core::types::Shape2dType::Polygon:
                    entity.set<components::PolygonShape2d>({});
                    break;
                case atmo::core::types::Shape2dType::None:
                    return;
            }

            spdlog::debug("Generating shape component for entity {} with shape {}", entity.name().c_str(), entity.world().to_expr(&physics_body.shape).c_str());
            createShapeForBody(entity, physics_body.body_id, physics_body);
        });

    world.system<components::Transform2d, components::PhysicsBody2d>("PhysicsBody2d_UpdateValuesFromPhysicsEngine")
        .kind(flecs::OnStore)
        .each([](flecs::entity e, components::Transform2d &transform, components::PhysicsBody2d &physics_body) {
            transform.position = b2Body_GetPosition(physics_body.body_id);
            transform.rotation = atmo::common::math::RadiansToDegrees(b2Rot_GetAngle(b2Body_GetRotation(physics_body.body_id)));
        });

    world.observer<components::PhysicsBody2d, components::Transform2d, components::Scene>("PhysicsBody2d_Init")
        .event(flecs::OnAdd)
        .term_at(2)
        .up()
        .each([](flecs::entity entity, components::PhysicsBody2d &physics_body, components::Transform2d &transform, components::Scene &scene) {
            spdlog::debug(
                "Creating PhysicsBody2d transform for entity {} with shape {}", entity.name().c_str(), entity.world().to_expr(&physics_body.shape).c_str());
            physics_body.body_def.position = transform.position;
            physics_body.body_def.rotation = b2MakeRot(atmo::common::math::DegreesToRadians(transform.rotation));
            physics_body.body_id = b2CreateBody(scene.world_id, &physics_body.body_def);
            createShapeForBody(entity, physics_body.body_id, physics_body);
        });

    // world.observer<components::PhysicsBody2d>("PhysicsBody2d_ShapeUpdate")
    //     .event(flecs::OnSet)
    //     .each([](flecs::entity entity, components::PhysicsBody2d &physics_body) {
    //         spdlog::debug("Updating shape for entity {} with shape {}", entity.name().c_str(), entity.world().to_expr(&physics_body.shape).c_str());
    //         createShapeForBody(entity, physics_body.body_id, physics_body);
    //     });

    world.observer<components::Transform2d, components::PhysicsBody2d, components::Scene>("PhysicsBody2d_TransformUpdate")
        .event(flecs::OnSet)
        .term_at(2)
        .up()
        .each([](flecs::entity e, components::Transform2d &transform, components::PhysicsBody2d &physics_body, components::Scene &scene) {
            spdlog::debug("Updating PhysicsBody2d transform for entity {} with shape {}", e.name().c_str(), e.world().to_expr(&physics_body.shape).c_str());
            b2Body_SetTransform(physics_body.body_id, transform.position, b2MakeRot(atmo::common::math::DegreesToRadians(transform.rotation)));
        });

#if !defined(ATMO_EXPORT)
    world.system<components::PhysicsBody2d, components::Transform2d, ComponentManager::Managed, components::Window>("Sprite2D_Render")
        .kind(flecs::OnValidate)
        .term_at(2)
        .up()
        .term_at(3)
        .up()
        .each([](flecs::entity e,
                 components::PhysicsBody2d &physics_body,
                 components::Transform2d &transform,
                 ComponentManager::Managed &manager,
                 components::Window &window) {
            auto wm = static_cast<atmo::impl::WindowManager *>(manager.ptr);

            // b2Body *body = b2Body_GetBody(physics_body.body_id);


            // SDL_RenderTextureRotated(
            //     wm->getRenderer(), texture, nullptr, &sprite.m_dest_rect, transform.g_rotation + transform.rotation, nullptr, SDL_FLIP_NONE);
        });
#endif
}

namespace
{
    static int _ = [] {
        atmo::core::ecs::Registry::AddBehaviorLoader(&setupPhysicsBody2dBehaviors);
        return 0;
    }();
} // namespace
