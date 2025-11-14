#pragma once

#include <flecs.h>
#include <map>
#include <string>

#include "components.hpp"
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

                void loadPrefabs();

            public:
                ECS();
                ~ECS() = default;
                void stop();
                void reset();

                void addPrefab(Prefab &prefab);

                Entity instantiatePrefab(const std::string &name, const std::string &instance_name = "");

                bool progress(float delta_time = 0.0f) const
                {
                    return m_world.progress(delta_time);
                }
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
