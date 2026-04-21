#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            struct StoreHandle {
                std::uint16_t index;
                std::uint16_t generation;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
