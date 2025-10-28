#pragma once

#include <flecs.h>
#include <map>
#include <string>

#include "components.hpp"

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            typedef flecs::entity Entity;

            class ECS
            {
            private:
                flecs::world m_world;
                std::map<std::string, flecs::entity> m_prefabs;

            public:
                ECS();
                ~ECS() = default;
                void stop();

                template <typename M> inline flecs::entity createManagedPrefab(const std::string &name)
                {
                    M::RegisterSystems(m_world);
                    auto prefab = m_world.prefab(name.c_str()).set(ComponentManager::Managed{ nullptr });

                    m_world.observer<ComponentManager::Managed>()
                        .event(flecs::OnAdd)
                        .with(flecs::IsA, prefab)
                        .each([](flecs::entity e, ComponentManager::Managed &m) { m.ptr = new M(e); });

                    m_world.observer<ComponentManager::Managed>()
                        .event(flecs::OnRemove)
                        .with(flecs::IsA, prefab)
                        .each([](flecs::entity e, ComponentManager::Managed &m) { delete m.ptr; });

                    return prefab;
                }

                void loadPrefabs();

                Entity instantiatePrefab(const std::string &name, const std::string &instance_name = "");

                bool progress(float delta_time = 0.0f) const
                {
                    return m_world.progress(delta_time);
                }
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
