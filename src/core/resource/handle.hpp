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
            struct StoreHandle
            {
                std::uint16_t index;
                std::uint16_t generation;
            };

            template<typename T>
            struct __Handle
            {
                std::string assetId; // complete path of the resource the handle handles
            };

            template<typename T>
            using Handle = std::shared_ptr<__Handle<T>>;
        } // namespace resource
    } // namespace core
} // namespace atmo
