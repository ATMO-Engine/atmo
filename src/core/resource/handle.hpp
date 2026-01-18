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
            struct Handle
            {
                std::string assetId; // complete path of the resource the handle handles
            };

            template<typename T>
            inline bool operator==(const Handle<T> &a, const Handle<T> &b)
            {
                return a.assetId == b.assetId;
            }

            template<typename T>
            inline bool operator!=(const Handle<T> &a, const Handle<T> &b)
            {
                return !(a == b);
            }

            template<typename T>
            inline bool operator<(const Handle<T> &a, const Handle<T> &b)
            {
                return a.assetId < b.assetId;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
