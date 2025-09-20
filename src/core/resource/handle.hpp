#pragma once

#include <cstdint>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            enum class ResourceType : std::uint8_t {
                PNG = 0,
                Script = 1
            };

            struct Handle
            {
                ResourceType type;
                std::uint16_t index;
                std::uint16_t generation;
                std::uint16_t frameToLive;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
