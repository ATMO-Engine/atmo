#pragma once

#include <string>

#include "luau/luau.hpp"

namespace atmo
{
    namespace core
    {
        namespace entities
        {
            typedef struct Script
            {
                std::string file;
                atmo::luau::Luau luau;
            } Script;
        } // namespace entities
    } // namespace core
} // namespace atmo
