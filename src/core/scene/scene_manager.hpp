#pragma once

#include <string_view>
#include <vector>

#include "project/file_system.hpp"
#include "scene.hpp"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            class SceneManager
            {
            public:
                SceneManager(flecs::world &world);
                ~SceneManager() = default;

                void changeScene(Scene *scene);
                void changeSceneToFile(std::string_view file_path);

            private:
                flecs::world &m_world;
                Scene *m_current;
                std::vector<Scene *> m_singletons;
            };
        } // namespace scene
    } // namespace core
} // namespace atmo
