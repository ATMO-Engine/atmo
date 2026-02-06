#include "box2d/box2d.h"
#include "box2d/math_functions.h"
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

    staticBody2dPrefab.set<components::Transform2d>({});
    staticBody2dPrefab.add<components::StaticBody2d>();

    auto physics_body_component = components::PhysicsBody2d{};
    physics_body_component.body_def.type = b2_staticBody;
    // physics_body_component.shape = atmo::core::resource::resources::Shape2d::Shape2dType::None;
    staticBody2dPrefab.set(physics_body_component);

    return staticBody2dPrefab;
}

namespace
{
    static int _ = [] {
        atmo::core::ecs::Registry::AddPrefabLoader(&createStaticBody2dPrefab);
        return 0;
    }();
} // namespace
