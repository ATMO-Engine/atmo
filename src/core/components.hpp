#pragma once

#include <cstdint>
#include <cstdlib>
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
            typedef struct Window
            {
                bool main = false;
                std::string title;
                types::vector2i size;
            } Window;

            typedef struct Script
            {
                std::string file;
                // atmo::luau::Luau luau;
            } Script;
        } // namespace components

        class ComponentManager
        {
        };
    } // namespace core
} // namespace atmo
