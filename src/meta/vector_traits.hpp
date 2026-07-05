#pragma once

#include <concepts>
#include <memory>
#include <vector>

#include "core/resource/subresources/subresource.hpp"

namespace atmo::meta
{
    template <typename T> struct IsStdVector : std::false_type { };

    template <typename T, typename A> struct IsStdVector<std::vector<T, A>> : std::true_type {
        using element_type = T;
    };

    template <typename T> struct IsSharedPtr : std::false_type { };

    template <typename T> struct IsSharedPtr<std::shared_ptr<T>> : std::true_type {
        using pointee = T;
    };

    template <typename T>
    concept IsSubResourceVector = IsStdVector<T>::value && IsSharedPtr<typename IsStdVector<T>::element_type>::value
        && std::derived_from<typename IsSharedPtr<typename IsStdVector<T>::element_type>::pointee, core::resource::SubResource>;

    template <typename T>
    concept IsPlainVector = IsStdVector<T>::value && !IsSubResourceVector<T>;

} // namespace atmo::meta
