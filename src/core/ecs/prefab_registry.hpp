#pragma once

#include <functional>
#include <vector>

#include "flecs.h"
#include "prefab.hpp"

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            class PrefabRegistry
            {
            public:
                static void AddPrefabLoader(const std::function<Prefab(flecs::world)> &loader);
                static const std::vector<std::function<Prefab(flecs::world)>> &GetLoaders();

            private:
                PrefabRegistry() = default;
                ~PrefabRegistry() = default;

                static PrefabRegistry &Instance();

                std::vector<std::function<Prefab(flecs::world)>> m_prefab_loaders;
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
