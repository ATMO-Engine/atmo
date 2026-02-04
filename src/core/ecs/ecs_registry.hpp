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
            class Registry
            {
            public:
                using PrefabLoaderFn = std::function<Prefab(flecs::world)>;
                using BehaviorLoaderFn = std::function<void(flecs::world)>;

                static void AddPrefabLoader(const PrefabLoaderFn &loader);
                static const std::vector<PrefabLoaderFn> &GetPrefabLoaders();

                static void AddBehaviorLoader(const BehaviorLoaderFn &loader);
                static const std::vector<BehaviorLoaderFn> &GetBehaviorLoaders();

            private:
                Registry() = default;
                ~Registry() = default;

                static Registry &Instance();

                std::vector<PrefabLoaderFn> m_prefab_loaders;
                std::vector<BehaviorLoaderFn> m_behavior_loaders;
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
