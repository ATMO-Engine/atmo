#pragma once

#include <flecs.h>
#include <spdlog/spdlog.h>

#include "core/components.hpp"
#include "core/input_manager.hpp"

namespace atmo
{
    namespace core
    {
        class Engine
        {
        private:
            flecs::world ecs;
            std::map<std::string, flecs::entity> prefabs;

        public:
            Engine()
            {
                ecs.init_builtin_components();
                components::register_core_components(ecs);
                load_prefabs();
            }

            void stop()
            {
                ecs.quit();
            }

            template <typename M> inline flecs::entity create_managed_prefab(const std::string &name)
            {
                M::registerSystems(ecs);
                auto prefab = ecs.prefab(name.c_str()).set(ComponentManager::Managed{ nullptr });

                ecs.observer<ComponentManager::Managed>()
                    .event(flecs::OnAdd)
                    .with(flecs::IsA, prefab)
                    .each([](flecs::entity e, ComponentManager::Managed &m) { m.ptr = new M(e); });

                ecs.observer<ComponentManager::Managed>()
                    .event(flecs::OnRemove)
                    .with(flecs::IsA, prefab)
                    .each([](flecs::entity e, ComponentManager::Managed &m) { delete m.ptr; });

                return prefab;
            }

            inline flecs::entity instantiate_prefab(const std::string &name, const std::string &instance_name = "")
            {
                if (prefabs.find(name) == prefabs.end()) {
                    throw std::runtime_error("Prefab not found: " + name);
                }

                flecs::entity instance;
                if (instance_name.empty()) {
                    instance = ecs.entity().is_a(prefabs.at(name));
                } else {
                    instance = ecs.entity(instance_name.c_str()).is_a(prefabs.at(name));
                }

                return instance;
            }

            void load_prefabs();

            inline const std::map<std::string, flecs::entity> &get_prefabs()
            {
                return prefabs;
            }

            inline flecs::world &get_ecs()
            {
                return ecs;
            }
        };
    } // namespace core
} // namespace atmo
