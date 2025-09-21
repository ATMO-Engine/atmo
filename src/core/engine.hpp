#pragma once

#include <any>
#include <flecs.h>
#include <spdlog/spdlog.h>

#include "core/components.hpp"
#include "luau/luau.hpp"

namespace atmo
{
    namespace core
    {
        class Engine
        {
        private:
            flecs::world ecs;
            std::map<std::string, flecs::entity> prefabs;
            inline static std::unordered_map<flecs::entity_t, ComponentManager> component_managers = {};

        public:
            Engine()
            {
                ecs.init_builtin_components();
                create_components();
                load_prefabs();
                init_systems();
            }

            template <typename T>
            inline void create_component(std::function<void(T &)> on_add = nullptr,
                                         std::function<void(T &)> on_remove = nullptr)
            {
                auto component = ecs.component<T>();
                if (on_add)
                    component.on_add(on_add);
                if (on_remove)
                    component.on_remove(on_remove);
            };

            template <typename T, typename Manager>
            inline void create_managed_component()
            {
                auto component = ecs.component<T>();
                component.on_add(
                    [this](flecs::entity e, T &c)
                    {
                        if (e.has(flecs::Prefab))
                            return;
                        Engine::component_managers[e.id()] = Manager(c);
                    });
                component.on_remove(
                    [this](flecs::entity e, T &c)
                    {
                        if (e.has(flecs::Prefab))
                            return;
                        Engine::component_managers.erase(e.id());
                    });
            }

            void create_components();
            void load_prefabs();
            void init_systems();

            inline const std::map<std::string, flecs::entity> &get_prefabs() { return prefabs; }
            inline flecs::world &get_ecs() { return ecs; }
        };
    } // namespace core
} // namespace atmo
