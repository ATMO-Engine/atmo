#include "scene.hpp"
#include "project/project_manager.hpp"

namespace atmo::core::ecs::entities
{
    void Scene::RegisterComponents(flecs::world *world)
    {
        world->component<components::Scene>();
    }

    void Scene::RegisterSystems(flecs::world *world)
    {
        world->observer<components::Scene>("Scene_Init2dPhysicsWorld").event(flecs::OnAdd).each([](flecs::entity e, components::Scene &scene) {
            b2WorldDef worldDef = b2DefaultWorldDef();
            auto gravity = atmo::project::ProjectManager::GetSettings().engine.gravity;
            worldDef.gravity = { gravity.x, gravity.y };
            scene.world_id = b2CreateWorld(&worldDef);
        });

        world->observer<components::Scene>("Scene_Destroy2dPhysicsWorld").event(flecs::OnRemove).each([](flecs::entity e, components::Scene &scene) {
            b2DestroyWorld(scene.world_id);
        });
    }

    void Scene::initialize()
    {
        Entity::initialize();

        setComponent<components::Scene>({});
    }

    void Scene::initFromFile(std::string_view file_path) {}

    void Scene::setSingleton(bool singleton)
    {
        auto scene = p_handle.get_ref<components::Scene>();
        scene->singleton = singleton;
        p_handle.modified<components::Scene>();
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Scene);
