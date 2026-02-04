#include "ecs.hpp"
#include <memory>
#include "SDL3/SDL_render.h"
#include "core/ecs/components.hpp"
#include "core/ecs/ecs_registry.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/scene/scene_manager.hpp"
#include "impl/window.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            ECS::ECS()
            {
                reset();
            }

            void ECS::stop()
            {
                m_world.quit();
            }

            void ECS::reset()
            {
                m_world.reset();
                m_scene_manager.setWorld(&m_world);

#if defined(ATMO_DEBUG)
                ECS_IMPORT(m_world, FlecsStats);
                m_world.set<flecs::Rest>({});
#endif

                m_world.init_builtin_components();
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

            flecs::entity ECS::createScene(const std::string &scene_name, bool singleton)
            {
                flecs::entity scene = instantiatePrefab("scene", scene_name).set<components::Scene>({ singleton });

                return scene;
            }

            void ECS::changeScene(flecs::entity scene)
            {
                flecs::entity current = m_scene_manager.getCurrentScene();

                if (current && scene == current)
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

            Entity ECS::instantiatePrefab(const std::string &name, const std::string &instance_name)
            {
                if (m_prefabs.find(name) == m_prefabs.end()) {
                    throw std::runtime_error("Prefab not found: " + name);
                }

                flecs::entity instance;
                if (instance_name.empty()) {
                    instance = m_world.entity().is_a(m_prefabs.at(name).entity);
                } else {
                    instance = m_world.entity(instance_name.c_str()).is_a(m_prefabs.at(name).entity);
                }

                return instance;
            }
        } // namespace ecs
    } // namespace core

} // namespace atmo
