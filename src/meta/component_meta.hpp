#pragma once

#include <tuple>
#include <type_traits>

#include "meta/field_descriptor.hpp"

namespace atmo::meta
{
    template <typename T> struct ComponentMeta;

    template <typename T>
    concept HasComponentMeta = requires {
        { ComponentMeta<T>::name } -> std::convertible_to<const char *>;
        ComponentMeta<T>::fields;
        requires std::is_same_v<std::remove_cvref_t<decltype(ComponentMeta<T>::fields)>, std::remove_cvref_t<decltype(ComponentMeta<T>::fields)>>;
    };

    template <typename T>
    concept HasComponentCategory = HasComponentMeta<T> && requires {
        { ComponentMeta<T>::category } -> std::convertible_to<const char *>;
    };

} // namespace atmo::meta
