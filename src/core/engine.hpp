#pragma once

#include <flecs.h>
#include <spdlog/spdlog.h>

#include "core/components.hpp"

namespace atmo
{
    namespace core
    {
        class Engine
        {
        private:
            flecs::world ecs;
            std::map<std::string, flecs::entity> prefabs;
            inline static std::unordered_map<flecs::entity_t, ComponentManager *> component_managers;

        public:
            Engine()
            {
                component_managers.clear();

                ecs.init_builtin_components();
                create_components();
                load_prefabs();
                init_systems();
            }

            void stop() { ecs.quit(); }

            template <typename T, typename Manager>
            inline flecs::entity create_managed_prefab(const std::string &name, T component)
            {
                auto prefab = ecs.prefab(name.c_str()).set<T>(component);

                ecs.observer()
                    .event(flecs::OnAdd)
                    .with(flecs::IsA, prefab)
                    .each(
                        [this](flecs::entity e)
                        {
                            const T &component = e.get<T>();
                            component_managers.emplace(e.id(), new Manager(component));
                        });

                ecs.observer()
                    .event(flecs::OnRemove)
                    .with(flecs::IsA, prefab)
                    .each(
                        [this](flecs::entity e)
                        {
                            if (Engine::component_managers.find(e.id()) == Engine::component_managers.end())
                                return;
                            delete Engine::component_managers[e.id()];
                            component_managers.erase(e.id());
                        });

                return prefab;
            }

            inline flecs::entity instantiate_prefab(const std::string &name, const std::string &instance_name = "")
            {
                if (prefabs.find(name) == prefabs.end()) {
                    spdlog::error("Prefab '{}' not found", name);
                    return flecs::entity();
                }

                flecs::entity instance;
                if (instance_name.empty()) {
                    instance = ecs.entity().is_a(prefabs.at(name));
                }
                else {
                    instance = ecs.entity(instance_name.c_str()).is_a(prefabs.at(name));
                }

                return instance;
            }

            void create_components();
            void load_prefabs();
            void init_systems();

            inline const std::map<std::string, flecs::entity> &get_prefabs() { return prefabs; }
            inline flecs::world &get_ecs() { return ecs; }
        };
    } // namespace core
} // namespace atmo
