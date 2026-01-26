#include "scene_manager.hpp"
#include <algorithm>
#include "core/ecs/components.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            void SceneManager::setWorld(flecs::world &world)
            {
                m_world = world;
            }

            flecs::entity SceneManager::loadSceneFromFile(std::string_view file_path)
            {
                project::File file = project::FileSystem::OpenFile(file_path);

                flecs::entity scene = m_world.entity();

                scene.from_json(file.readAll().c_str());

                if (!scene.is_valid()) {
                    spdlog::error("Failed to load scene from file: {}", file_path);
                    throw std::runtime_error("Failed to load scene from file: " + std::string(file_path));
                }

                return scene;
            }

            void SceneManager::loadSingleton(std::string_view file_path)
            {
                flecs::entity singleton = loadSceneFromFile(file_path);
                m_singletons.push_back(singleton);
            }

            flecs::entity SceneManager::getCurrentScene() const
            {
                return m_current;
            }

            flecs::entity SceneManager::getRoot()
            {
                static flecs::entity root = m_world.entity("_Root");
                return root;
            }

            void SceneManager::changeScene(flecs::entity scene)
            {
                if (std::find(m_singletons.begin(), m_singletons.end(), scene) != m_singletons.end()) {
                    spdlog::error("Cannot change to a singleton scene: {}", scene.name().c_str());
                    throw SwitchToSingletonSceneException();
                }

                flecs::entity old_scene = m_current;
                m_current = scene;
                m_current.child_of(getRoot());
                if (m_initialized)
                    old_scene.destruct();
                m_initialized = true;
            }

            void SceneManager::changeSceneToFile(std::string_view file_path)
            {
                flecs::entity scene = loadSceneFromFile(file_path);

                changeScene(scene);
            }
        } // namespace scene
    } // namespace core
} // namespace atmo
