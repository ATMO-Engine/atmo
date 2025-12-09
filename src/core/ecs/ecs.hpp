#pragma once

#include <flecs.h>
#include <map>
#include <string>

#include "components.hpp"
#include "core/scene/scene_manager.hpp"
#include "prefab.hpp"

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            using Entity = flecs::entity;

            class ECS
            {
            private:
                flecs::world m_world;
                std::map<std::string, Prefab> m_prefabs;
                scene::SceneManager m_scene_manager;

                void loadPrefabs();

            public:
                ECS();
                ~ECS() = default;
                void stop();
                void reset();

                scene::Scene createScene(const std::string &scene_name, bool singleton);
                void changeScene(scene::Scene scene);
                void changeSceneToFile(std::string_view scene_path);

                void addPrefab(Prefab &prefab);

                Entity instantiatePrefab(scene::Scene scene, const std::string &name, const std::string &instance_name = "");

                bool progress(float delta_time = 0.0f) const
                {
                    return m_world.progress(delta_time);
                }
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
