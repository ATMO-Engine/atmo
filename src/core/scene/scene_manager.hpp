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
            class SceneManager
            {
            private:
                flecs::world m_world;
                flecs::entity m_current;
                bool m_initialized{ false };
                std::vector<flecs::entity> m_singletons;

                flecs::entity loadSceneFromFile(std::string_view file_path);
                flecs::entity getRoot();

            public:
                ~SceneManager() = default;

                void setWorld(flecs::world &world);

                flecs::entity getCurrentScene() const;

                void changeScene(flecs::entity scene);
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
