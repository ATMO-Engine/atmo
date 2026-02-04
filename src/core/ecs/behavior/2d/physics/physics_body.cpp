#include <unordered_map>
#include "core/ecs/components.hpp"
#include "core/ecs/ecs.hpp"
#include "core/ecs/ecs_registry.hpp"
#include "core/types.hpp"
#include "flecs.h"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

void setupPhysicsBody2dBehaviors(flecs::world world)
{
    using namespace atmo::core;

    world.observer<components::PhysicsBody2D>("Shape2d_ComponentGenerator").event(flecs::OnSet).each([](flecs::entity e, components::PhysicsBody2D &t) {
        spdlog::debug("PhysicsBody2D OnSet triggered for entity: {}", e.name().c_str());
        spdlog::debug(" - {}", e.world().to_expr(&t.shape).c_str());
        switch (t.shape) {
            case atmo::core::types::Shape2DType::Rectangle:
                {
                    if (e.has<components::RectangleShape2d>())
                        return;
                }
            case atmo::core::types::Shape2DType::Circle:
                {
                    if (e.has<components::CircleShape2d>())
                        return;
                }
            case atmo::core::types::Shape2DType::Capsule:
                {
                    if (e.has<components::CapsuleShape2d>())
                        return;
                }
            case atmo::core::types::Shape2DType::Polygon:
                {
                    if (e.has<components::PolygonShape2d>())
                        return;
                }
            case atmo::core::types::Shape2DType::None:
                break;
        }

        e.remove<components::RectangleShape2d>();
        e.remove<components::CircleShape2d>();
        e.remove<components::CapsuleShape2d>();
        e.remove<components::PolygonShape2d>();

        switch (t.shape) {
            case atmo::core::types::Shape2DType::Rectangle:
                e.set<components::RectangleShape2d>({});
                break;
            case atmo::core::types::Shape2DType::Circle:
                e.set<components::CircleShape2d>({});
                break;
            case atmo::core::types::Shape2DType::Capsule:
                e.set<components::CapsuleShape2d>({});
                break;
            case atmo::core::types::Shape2DType::Polygon:
                e.set<components::PolygonShape2d>({});
                break;
            case atmo::core::types::Shape2DType::None:
                break;
        }
    });
}

namespace
{
    static int _ = [] {
        atmo::core::ecs::Registry::AddBehaviorLoader(&setupPhysicsBody2dBehaviors);
        return 0;
    }();
} // namespace
