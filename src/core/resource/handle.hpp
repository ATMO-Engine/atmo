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
                std::string path; // complete path of the resource the handle handles
                std::uint16_t index;
                std::uint16_t generation;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
