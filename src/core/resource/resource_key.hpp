#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

namespace atmo::core::resource
{
    template <typename T> struct ResourceKey {
        using type = std::string;
    };

    template <typename Key> struct ResourceKeyHash;

    template <> struct ResourceKeyHash<std::string> {
        size_t operator()(const std::string &key) const noexcept
        {
            return std::hash<std::string>{}(key);
        }
    };
} // namespace atmo::core::resource
