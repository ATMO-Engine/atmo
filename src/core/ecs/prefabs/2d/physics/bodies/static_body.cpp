#include "core/ecs/components.hpp"
#include "core/ecs/ecs.hpp"
#include "core/ecs/ecs_registry.hpp"
#include "flecs.h"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

atmo::core::ecs::Prefab createStaticBody2dPrefab(flecs::world world)
{
    using namespace atmo::core;
    auto staticBody2dPrefab = ecs::Prefab(world, "static_body_2d");

    staticBody2dPrefab.set(components::PhysicsBody2D{});

    staticBody2dPrefab.set(components::StaticBody2D{});
    auto body = staticBody2dPrefab.entity.get_ref<components::PhysicsBody2D>();
    body->body_def.type = b2_staticBody;
    body->shape = types::Shape2DType::Rectangle;

    return staticBody2dPrefab;
}

namespace
{
    static int _ = [] {
        atmo::core::ecs::Registry::AddPrefabLoader(&createStaticBody2dPrefab);
        return 0;
    }();
} // namespace
