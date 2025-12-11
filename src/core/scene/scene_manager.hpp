#pragma once

#include <string_view>
#include <vector>

#include "project/file_system.hpp"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            using Scene = flecs::entity;

            class SceneManager
            {
            private:
                flecs::world m_world;
                Scene m_current;
                std::vector<Scene> m_singletons;

                Scene loadSceneFromFile(std::string_view file_path);

            public:
                ~SceneManager() = default;

                void setWorld(flecs::world &world);

                Scene getCurrentScene() const;

                void changeScene(Scene scene);
                void changeSceneToFile(std::string_view file_path);
                void loadSingleton(std::string_view file_path);

                class SwitchToSingletonSceneException : public std::exception
                {
                public:
                    const char *what() const noexcept override
                    {
                        return "Cannot switch to a singleton scene.";
                    }
                };
            };
        } // namespace scene
    } // namespace core
} // namespace atmo
