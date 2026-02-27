#pragma once

#include <exception>
#include <flecs.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "components.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/scene/scene_manager.hpp"
#include "prefab.hpp"

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            class ECS
            {
            private:
                flecs::world m_world;
                std::unordered_map<std::string, Prefab> m_prefabs;
                scene::SceneManager m_scene_manager;

                void loadPrefabs();
                void addPrefab(Prefab &prefab);

            public:
                ECS();
                ~ECS() = default;
                void stop();
                void reset();

                std::unique_ptr<entities::Scene> createScene(const std::string &scene_name, bool singleton);
                void changeScene(entities::Scene scene);
                void changeSceneToFile(std::string_view scene_path);

                bool progress(float delta_time = 0.0f) const
                {
                    return m_world.progress(delta_time);
                }
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
