#pragma once

#include <cstdint>
#include <string>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            struct Handle
            {
                std::string path;
                std::uint16_t index;
                std::uint16_t generation;
                std::uint16_t frameToLive;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
