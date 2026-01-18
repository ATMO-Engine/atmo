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
                return a.path == b.path && a.index == b.index && a.generation == b.generation;
            }

            template<typename T>
            inline bool operator!=(const Handle<T> &a, const Handle<T> &b)
            {
                return !(a == b);
            }

            template<typename T>
            inline bool operator<(const Handle<T> &a, const Handle<T> &b)
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
