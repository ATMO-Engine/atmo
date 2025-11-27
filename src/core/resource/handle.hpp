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

            inline bool operator==(const handle &a, const handle &b)
            {
                return a.path == b.path && a.index == b.index && a.generation == b.generation;
            }

            inline bool operator!=(const handle &a, const handle &b)
            {
                return !(a == b);
            }

            inline bool operator<(const handle &a, const handle &b)
            {
                if (a.path != b.path)
                    return a.path < b.path;
                if (a.index != b.index)
                    return a.index < b.index;
                return a.generation < b.generation;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
