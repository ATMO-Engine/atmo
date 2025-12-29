#pragma once

#include <cstdint>
#include <string>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            typedef struct Handle {
                std::string path;
                std::uint16_t index;
                std::uint16_t generation;
                std::uint16_t frame_to_live;
            } handle;
        } // namespace resource
    } // namespace core
} // namespace atmo
