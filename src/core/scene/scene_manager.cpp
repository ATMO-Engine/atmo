#include "scene_manager.hpp"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            SceneManager::SceneManager(flecs::world &world) : m_world(world), m_current(nullptr) {}

            void SceneManager::changeScene(Scene *scene)
            {
                if (m_current != nullptr) {
                    m_current->destroy();
                    delete m_current;
                }

                m_current = scene;
                m_current->init();
            }

            void SceneManager::changeSceneToFile(std::string_view file_path)
            {
                project::File file = project::FileSystem::OpenFile(file_path);

                Scene *scene = Scene::LoadFromFile(m_world, file);

                changeScene(scene);
            }
        } // namespace scene
    } // namespace core
} // namespace atmo
