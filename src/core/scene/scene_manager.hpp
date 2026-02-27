#pragma once

#include <string_view>
#include <vector>

#include "core/ecs/entities/scene/scene.hpp"
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
                flecs::world *m_world;
                ecs::entities::Scene m_current;
                bool m_initialized{ false };
                std::vector<ecs::entities::Scene> m_singletons;

                ecs::entities::Scene loadSceneFromFile(std::string_view file_path);
                ecs::entities::Scene getRoot();

            public:
                ~SceneManager() = default;

                void setWorld(flecs::world *world);

                ecs::entities::Scene getCurrentScene() const;

                void changeScene(ecs::entities::Scene scene);
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
