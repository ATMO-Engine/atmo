#pragma once

#include <cstdint>
#include <cstdlib>
#include <flecs.h>
#include <spdlog/spdlog.h>
#include <string>

#include "core/types.hpp"
#include "luau/luau.hpp"

namespace atmo
{
    namespace core
    {
        namespace components
        {
            typedef struct Window {
                bool main = false;
                std::string title;
                types::vector2i size;
            } Window;

            typedef struct Script {
                std::string file;
            } Script;
        } // namespace components

        class ComponentManager
        {
        public:
            virtual ~ComponentManager() = default;

            template <typename Component>
            static void registerSystems(
                flecs::world &ecs, std::unordered_map<flecs::entity_t, ComponentManager *> component_managers
            )
            {
            }
        };
    } // namespace core
} // namespace atmo
