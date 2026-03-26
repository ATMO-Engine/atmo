#pragma once

#include <tuple>
#include <type_traits>

#include "flecs.h"

#include "meta/component_meta.hpp"
#include "meta/field_descriptor.hpp"

namespace atmo::meta
{
    namespace detail
    {
        template <typename Owner, auto MemberPtr> void register_flecs_field(flecs::component<Owner> &c, const FieldDescriptor<Owner, MemberPtr> &fd)
        {
            using FieldT = typename FieldDescriptor<Owner, MemberPtr>::field_type;
            c.template member<FieldT>(fd.name);
        }

        template <typename Owner, typename Tuple, std::size_t... I>
        void register_flecs_fields(flecs::component<Owner> &c, const Tuple &fields, std::index_sequence<I...>)
        {
            (register_flecs_field(c, std::get<I>(fields)), ...);
        }

    } // namespace detail

    template <HasComponentMeta T> void register_flecs_meta(flecs::world &world)
    {
        auto c = world.component<T>();

        constexpr auto &fields = ComponentMeta<T>::fields;
        constexpr auto field_count = std::tuple_size_v<std::remove_cvref_t<decltype(fields)>>;

        detail::register_flecs_fields(c, fields, std::make_index_sequence<field_count>{});
    }

} // namespace atmo::meta
