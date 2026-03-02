#include "scene.hpp"
#include "box2d/box2d.h"
#include "project/project_manager.hpp"

namespace atmo::core::ecs::entities
{
    void Scene::RegisterComponents(flecs::world *world)
    {
        world->component<components::Scene>();
    }

    void Scene::RegisterSystems(flecs::world *world)
    {
        world->observer<components::Scene>("Scene_Destroy2dPhysicsWorld").event(flecs::OnRemove).each([](flecs::entity e, components::Scene &scene) {
            if (b2World_IsValid(scene.world_id)) {
                b2DestroyWorld(scene.world_id);
            }
        });
    }

    void Scene::initialize()
    {
        Entity::initialize();

        setComponent<components::Scene>({});
        auto scene = p_handle.get_ref<components::Scene>();
        b2WorldDef worldDef = b2DefaultWorldDef();
        auto gravity = atmo::project::ProjectManager::GetSettings().engine.gravity;
        worldDef.gravity = { gravity.x, gravity.y };
        scene->world_id = b2CreateWorld(&worldDef);
    }

    void Scene::initFromFile(std::string_view file_path) {}

    void Scene::setSingleton(bool singleton)
    {
        auto scene = p_handle.get_ref<components::Scene>();
        scene->singleton = singleton;
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Scene);
