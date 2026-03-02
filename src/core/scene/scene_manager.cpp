#include "scene_manager.hpp"
#include <algorithm>
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            void SceneManager::setWorld(flecs::world *world)
            {
                m_world = world;
            }

            std::shared_ptr<ecs::entities::Scene> SceneManager::loadSceneFromFile(std::string_view file_path)
            {
                project::File file = project::FileSystem::OpenFile(file_path);

                auto scene = ecs::EntityRegistry::Create<ecs::entities::Scene>("Entity::Scene");

                scene->loadFromJson(file.readAll().c_str());

                if (!scene->isAlive()) {
                    spdlog::error("Failed to load scene from file: {}", file_path);
                    throw std::runtime_error("Failed to load scene from file: " + std::string(file_path));
                }

                return scene;
            }

            void SceneManager::loadSingleton(std::string_view file_path)
            {
                auto singleton = loadSceneFromFile(file_path);
                m_singletons.push_back(singleton);
            }

            std::shared_ptr<ecs::entities::Scene> SceneManager::getCurrentScene() const
            {
                return m_current;
            }

            std::shared_ptr<ecs::entities::Scene> SceneManager::getRoot()
            {
                static auto root = std::make_shared<ecs::entities::Scene>(m_world->entity("_Root"));
                return root;
            }

            void SceneManager::changeScene(std::shared_ptr<ecs::entities::Scene> scene)
            {
                if (std::find(m_singletons.begin(), m_singletons.end(), scene) != m_singletons.end()) {
                    spdlog::error("Cannot change to a singleton scene: {}", scene->name());
                    throw SwitchToSingletonSceneException();
                }

                std::shared_ptr<ecs::entities::Scene> old_scene = m_current;
                m_current = scene;
                m_current->isChildOf(*getRoot());
                if (m_initialized)
                    old_scene->destroy();
                m_initialized = true;
            }

            void SceneManager::changeSceneToFile(std::string_view file_path)
            {
                auto scene = loadSceneFromFile(file_path);

                changeScene(scene);
            }
        } // namespace scene
    } // namespace core
} // namespace atmo
