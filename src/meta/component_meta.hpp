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

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector2> {
    static constexpr const char *name = "Vector2";
    static constexpr auto fields =
        std::make_tuple(atmo::meta::field<&atmo::core::types::Vector2::x>("x"), atmo::meta::field<&atmo::core::types::Vector2::y>("y"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector2i> {
    static constexpr const char *name = "Vector2i";
    static constexpr auto fields =
        std::make_tuple(atmo::meta::field<&atmo::core::types::Vector2i::x>("x"), atmo::meta::field<&atmo::core::types::Vector2i::y>("y"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector3> {
    static constexpr const char *name = "Vector3";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Vector3::x>("x"), atmo::meta::field<&atmo::core::types::Vector3::y>("y"),
        atmo::meta::field<&atmo::core::types::Vector3::z>("z"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector3i> {
    static constexpr const char *name = "Vector3i";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Vector3i::x>("x"), atmo::meta::field<&atmo::core::types::Vector3i::y>("y"),
        atmo::meta::field<&atmo::core::types::Vector3i::z>("z"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector4> {
    static constexpr const char *name = "Vector4";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Vector4::x>("x"), atmo::meta::field<&atmo::core::types::Vector4::y>("y"),
        atmo::meta::field<&atmo::core::types::Vector4::z>("z"), atmo::meta::field<&atmo::core::types::Vector4::w>("w"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector4i> {
    static constexpr const char *name = "Vector4i";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Vector4i::x>("x"), atmo::meta::field<&atmo::core::types::Vector4i::y>("y"),
        atmo::meta::field<&atmo::core::types::Vector4i::z>("z"), atmo::meta::field<&atmo::core::types::Vector4i::w>("w"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Color> {
    static constexpr const char *name = "Color";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Color::r>("r"), atmo::meta::field<&atmo::core::types::Color::g>("g"),
        atmo::meta::field<&atmo::core::types::Color::b>("b"), atmo::meta::field<&atmo::core::types::Color::a>("a"));
};
