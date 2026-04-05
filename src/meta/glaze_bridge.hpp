#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include "glaze/glaze.hpp"

#include "meta/component_meta.hpp"
#include "meta/field_descriptor.hpp"


namespace atmo::meta::detail
{
    template <typename Owner, auto MemberPtr> constexpr auto field_to_glz_pair(const FieldDescriptor<Owner, MemberPtr> &fd)
    {
        return std::make_tuple(fd.name, MemberPtr);
    }

    template <typename Tuple, std::size_t... I> constexpr auto fields_to_glz_tuple(const Tuple &fields, std::index_sequence<I...>)
    {
        return std::tuple_cat(field_to_glz_pair(std::get<I>(fields))...);
    }

    template <typename... Args> constexpr auto apply_glz_object(std::tuple<Args...> args)
    {
        return std::apply([](auto &&...a) { return glz::object(std::forward<decltype(a)>(a)...); }, args);
    }

    template <atmo::meta::HasComponentMeta T> constexpr auto make_glz_value()
    {
        constexpr auto &fields = ComponentMeta<T>::fields;
        constexpr auto field_count = std::tuple_size_v<std::remove_cvref_t<decltype(fields)>>;

        auto flat = fields_to_glz_tuple(fields, std::make_index_sequence<field_count>{});
        return apply_glz_object(std::move(flat));
    }

} // namespace atmo::meta::detail

template <typename T>
    requires atmo::meta::HasComponentMeta<T>
struct glz::meta<T> {
    static constexpr auto value = atmo::meta::detail::make_glz_value<T>();
};
