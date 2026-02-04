#include "core/ecs/ecs.hpp"
#include "core/ecs/prefab_registry.hpp"
#include "flecs.h"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

atmo::core::ecs::Prefab createScenePrefab(flecs::world world)
{
    using namespace atmo::core;
    auto scenePrefab = ecs::Prefab(world, "scene").set(components::Scene{ false });

    world.observer<components::Scene>("Scene_Init2dPhysicsWorld").event(flecs::OnAdd).each([](flecs::entity e, components::Scene &scene) {
        b2WorldDef worldDef = b2DefaultWorldDef();
        auto gravity = atmo::project::ProjectManager::GetSettings().engine.gravity;
        worldDef.gravity = { gravity.x, gravity.y };
        scene.world_id = b2CreateWorld(&worldDef);
    });

    world.observer<components::Scene>("Scene_Destroy2dPhysicsWorld").event(flecs::OnRemove).each([](flecs::entity e, components::Scene &scene) {
        b2DestroyWorld(scene.world_id);
    });

    return scenePrefab;
}

namespace
{
    static int _ = [] {
        atmo::core::ecs::PrefabRegistry::AddPrefabLoader(&createScenePrefab);
        return 0;
    }();
} // namespace
