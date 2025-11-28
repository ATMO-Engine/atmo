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
            struct __Handle
            {
                std::string path; // complete path of the resource the handle handles
                std::uint16_t index;
                std::uint16_t generation;
            };

            using Handle = std::shared_ptr<__Handle>;
        } // namespace resource
    } // namespace core
} // namespace atmo
