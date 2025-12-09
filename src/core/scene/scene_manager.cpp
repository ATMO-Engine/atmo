#include "scene_manager.hpp"
#include "core/ecs/components.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            SceneManager::SceneManager(flecs::world &world) : m_world(world), m_current(nullptr) {}

            Scene SceneManager::loadSceneFromFile(std::string_view file_path)
            {
                project::File file = project::FileSystem::OpenFile(file_path);

                Scene scene = m_world.entity();
                scene.from_json(file.readAll().c_str());

                if (!scene.is_valid() || !scene.has<components::Scene>()) {
                    spdlog::error("Failed to load scene from file: {}", file_path);
                    throw std::runtime_error("Failed to load scene from file: " + std::string(file_path));
                }

                return scene;
            }

            void SceneManager::loadSingleton(std::string_view file_path)
            {
                Scene singleton = loadSceneFromFile(file_path);
                m_singletons.push_back(singleton);
            }

            Scene SceneManager::getCurrentScene() const
            {
                return m_current;
            }

            void SceneManager::changeScene(Scene scene)
            {
                if (m_current.is_valid()) {
                    m_current.destruct();
                }

                m_current = scene;
            }

            void SceneManager::changeSceneToFile(std::string_view file_path)
            {
                Scene scene = loadSceneFromFile(file_path);

                changeScene(scene);
            }
        } // namespace scene
    } // namespace core
} // namespace atmo
