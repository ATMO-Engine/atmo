#include "core/ecs/ecs.hpp"
#include "core/ecs/ecs_registry.hpp"
#include "flecs.h"
#include "project/project_manager.hpp"

void setupTransform2dBehaviors(flecs::world world)
{
    using namespace atmo::core;
    world.system<components::Transform2d, components::Transform2d>("Transform2d_GenerateGlobal")
        .kind(flecs::PreUpdate)
        .term_at(1)
        .up()
        .each([](flecs::entity e, components::Transform2d &t, const components::Transform2d &parent_t) {
            t.g_position = { parent_t.g_position.x + t.position.x, parent_t.g_position.y + t.position.y };
            t.g_rotation = parent_t.g_rotation + t.rotation;
            t.g_scale = { parent_t.g_scale.x * t.scale.x, parent_t.g_scale.y * t.scale.y };
        });
}

namespace
{
    static int _ = [] {
        atmo::core::ecs::Registry::AddBehaviorLoader(&setupTransform2dBehaviors);
        return 0;
    }();
} // namespace
