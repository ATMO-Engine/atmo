#include "scene.hpp"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            void Scene::init()
            {
                // TODO: Initialize the Box2D world using the ProjectManager's m_settings (ProjectSettings)
            }

            void Scene::destroy()
            {
                b2DestroyWorld(m_box2d_world);
            }

            Scene *Scene::LoadFromFile(flecs::world &world, project::File &file)
            {
                Scene *scene = new Scene(world, world.entity());

                // TODO: Create SceneData structure in scene.hpp, it should map prefabs and store their component's parameters
                // TODO: Use glaze to read file, it should store a json object containing a SceneData structure
                // TODO: Use structure to populate world with correct entity hierarchy

                return scene;
            }
        } // namespace scene
    } // namespace core
} // namespace atmo
