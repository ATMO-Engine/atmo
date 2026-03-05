#include "ecs.hpp"
#include <memory>
#include "SDL3/SDL_render.h"
#include "core/ecs/components.hpp"
#include "core/ecs/ecs_registry.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/scene/scene_manager.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs
{
    ECS::ECS()
    {
        reset();
    }

    void ECS::stop()
    {
        EntityRegistry::UnregisterAll(&m_world);
        m_world.quit();
    }

    void ECS::reset()
    {
        m_world.reset();
        m_world.init_builtin_components();

        m_scene_manager.setWorld(&m_world);
        EntityRegistry::SetWorld(&m_world);

#if defined(ATMO_DEBUG)
        ECS_IMPORT(m_world, FlecsStats);
        m_world.set<flecs::Rest>({});
#endif

        components::register_core_components(m_world);
        loadPrefabs();
    }

    void ECS::loadPrefabs()
    {
        for (auto loader : Registry::GetPrefabLoaders()) {
            auto p = loader(m_world);
            addPrefab(p);
        };

        for (auto loader : Registry::GetBehaviorLoaders()) {
            loader(m_world);
        };
    }

    std::shared_ptr<entities::Scene> ECS::createScene(const std::string &scene_name, bool singleton)
    {
        auto scene = EntityRegistry::Create<entities::Scene>("Entity::Scene");
        scene->setSingleton(singleton);

        return scene;
    }

    void ECS::changeScene(std::shared_ptr<entities::Scene> scene)
    {
        auto current = m_scene_manager.getCurrentScene();

        if (scene == current)
            return;

        try {
            m_scene_manager.changeScene(scene);
        } catch (const scene::SceneManager::SwitchToSingletonSceneException &e) {
            return;
        }
    }

    void ECS::changeSceneToFile(std::string_view scene_path)
    {
        m_scene_manager.changeSceneToFile(scene_path);
    }

    void ECS::addPrefab(Prefab &prefab)
    {
        m_prefabs.emplace(prefab.name, prefab);
    }
} // namespace atmo::core::ecs
