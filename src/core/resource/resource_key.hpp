#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#include "SDL3/SDL_render.h"

namespace atmo::core::resource
{
    template <typename T> struct GpuResource : std::false_type {
    };

    template <typename T> struct ResourceKey {
        using type = std::string;
    };

    template <> struct ResourceKey<SDL_Texture> {
        using type = std::pair<std::string, SDL_Renderer *>;
    };

    template <> struct GpuResource<SDL_Texture> : std::true_type {
    };

    template <typename Key> struct ResourceKeyHash;

    template <> struct ResourceKeyHash<std::string> {
        size_t operator()(const std::string &key) const noexcept
        {
            return std::hash<std::string>{}(key);
        }
    };

    template <> struct ResourceKeyHash<std::pair<std::string, SDL_Renderer *>> {
        size_t operator()(const std::pair<std::string, SDL_Renderer *> &key) const noexcept
        {
            const size_t h1 = std::hash<std::string>{}(key.first);
            const size_t h2 = std::hash<void *>{}(key.second);
            return h1 ^ (h2 << 1);
        }
    };
} // namespace atmo::core::resource
