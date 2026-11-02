#pragma once

#include <string>
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
                flecs::world *m_world{ nullptr };
                std::shared_ptr<ecs::entities::Scene> m_current;
                bool m_initialized{ false };
                std::vector<std::shared_ptr<ecs::entities::Scene>> m_singletons;

                /**
                 * @brief Create a scene from a .ascn scene file. Falls back to an empty scene if the file can't be used.
                 *
                 * @param file_path Path of the .ascn file.
                 * @param scene_name Set to the name saved in the file, left untouched on failure. The scene keeps its
                 *                   default name until the caller renames it.
                 * @return std::shared_ptr<ecs::entities::Scene> The created scene.
                 */
                std::shared_ptr<ecs::entities::Scene> loadSceneFromFile(std::string_view file_path, std::string &scene_name);
                std::shared_ptr<ecs::entities::Scene> getRoot();

            public:
                ~SceneManager() = default;

                void setWorld(flecs::world *world);

                std::shared_ptr<ecs::entities::Scene> getCurrentScene() const;

                void changeScene(std::shared_ptr<ecs::entities::Scene> scene);
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
