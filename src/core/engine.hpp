#pragma once

#include <flecs.h>
#include <spdlog/spdlog.h>

#include "core/components.hpp"
#include "core/input_manager.hpp"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"

namespace atmo
{
    namespace core
    {
        class Engine
        {
        private:
            flecs::world ecs;
            std::map<std::string, flecs::entity> prefabs;
            std::unordered_map<flecs::entity_t, ComponentManager *> component_managers;

        public:
            Engine()
            {
                component_managers.clear();

                ecs.init_builtin_components();
                load_prefabs();
            }

            void stop()
            {
                ecs.quit();
            }

            template <typename T, typename Manager>
            inline flecs::entity create_managed_prefab(const std::string &name, T component)
            {
                Manager::template registerSystems<T>(ecs, component_managers);
                auto prefab = ecs.prefab(name.c_str()).set<T>(component);

                ecs.observer().event(flecs::OnAdd).with(flecs::IsA, prefab).each([this](flecs::entity e) {
                    const T &component = e.get<T>();
                    component_managers.emplace(e.id(), new Manager(component, e));
                });

                ecs.observer().event(flecs::OnRemove).with(flecs::IsA, prefab).each([this](flecs::entity e) {
                    if (ecs.should_quit())
                        return;
                    if (Engine::component_managers.find(e.id()) == Engine::component_managers.end())
                        return;
                    auto mngr = Engine::component_managers[e.id()];
                    component_managers.erase(e.id());
                    delete mngr;
                });

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

            ComponentManager *get_component_manager(flecs::entity_t id)
            {
                if (component_managers.find(id) == component_managers.end())
                    return nullptr;

                return component_managers[id];
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
