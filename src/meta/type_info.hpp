#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "flecs.h"
#include "glaze/glaze.hpp"

#include "meta/component_meta.hpp"
#include "meta/field_descriptor.hpp"
#include "meta/glaze_bridge.hpp"

namespace atmo::meta
{
    struct FieldInfo {
        const char *name = nullptr;
        const char *label = nullptr;
        const char *tooltip = nullptr;
        const char *category = nullptr;
        const char *widget = nullptr;
        bool read_only = false;
        bool has_range = false;
        float range_min = 0.0f;
        float range_max = 0.0f;
        std::size_t offset = 0;
        std::size_t size = 0;

        void (*get)(const void *component, void *out) = nullptr;
        void (*set)(void *component, const void *value) = nullptr;
    };

    struct TypeInfo {
        const char *name = nullptr;
        const char *category = nullptr;
        std::size_t size = 0;
        std::uint64_t flecs_id = 0;
        std::vector<FieldInfo> fields;

        void (*register_flecs)(flecs::world &) = nullptr;
        std::uint64_t (*resolve_flecs_id)(flecs::world &) = nullptr;
        std::string (*to_json)(const void *component, bool prettify) = nullptr;
        bool (*from_json)(void *component, std::string_view json) = nullptr;
    };

    namespace detail
    {
        template <typename Owner, auto MemberPtr> std::size_t member_offset()
        {
            alignas(Owner) std::byte storage[sizeof(Owner)]{};
            auto *owner = reinterpret_cast<Owner *>(&storage);
            auto *field_addr = reinterpret_cast<std::byte *>(&(owner->*MemberPtr));
            return static_cast<std::size_t>(field_addr - &storage[0]);
        }

        template <typename Owner, auto MemberPtr> FieldInfo make_field_info(const FieldDescriptor<Owner, MemberPtr> &fd)
        {
            using FieldT = typename FieldDescriptor<Owner, MemberPtr>::field_type;

            FieldInfo fi;
            fi.name = fd.name;
            fi.label = fd.label;
            fi.tooltip = fd.tooltip;
            fi.category = fd.category;
            fi.widget = fd.widget;
            fi.read_only = fd.read_only;
            fi.has_range = fd.has_range;
            fi.range_min = fd.range_min;
            fi.range_max = fd.range_max;
            fi.offset = member_offset<Owner, MemberPtr>();
            fi.size = sizeof(FieldT);

            fi.get = [](const void *component, void *out) {
                const auto *owner = static_cast<const Owner *>(component);
                *static_cast<FieldT *>(out) = owner->*MemberPtr;
            };

            if (!fd.read_only) {
                fi.set = [](void *component, const void *value) {
                    auto *owner = static_cast<Owner *>(component);
                    owner->*MemberPtr = *static_cast<const FieldT *>(value);
                };
            }

            return fi;
        }

        template <typename Tuple, std::size_t... I> void fill_fields(const Tuple &tuple, std::vector<FieldInfo> &out, std::index_sequence<I...>)
        {
            (out.push_back(make_field_info(std::get<I>(tuple))), ...);
        }

    } // namespace detail

    template <HasComponentMeta T> TypeInfo make_type_info()
    {
        TypeInfo ti;
        ti.name = ComponentMeta<T>::name;
        ti.size = sizeof(T);

        if constexpr (HasComponentCategory<T>) {
            ti.category = ComponentMeta<T>::category;
        }

        constexpr auto &fields = ComponentMeta<T>::fields;
        constexpr auto field_count = std::tuple_size_v<std::remove_cvref_t<decltype(fields)>>;

        ti.fields.reserve(field_count);
        detail::fill_fields(fields, ti.fields, std::make_index_sequence<field_count>{});

        ti.resolve_flecs_id = [](flecs::world &w) -> std::uint64_t { return w.component<T>().id(); };

        ti.to_json = [](const void *component, bool prettify) -> std::string {
            auto result = glz::write_json<{ .prettify = prettify }>(*static_cast<const T *>(component));
            return result.value_or("");
        };

        ti.from_json = [](void *component, std::string_view json) -> bool {
            auto err = glz::read_json(*static_cast<T *>(component), json);
            return !err;
        };

        return ti;
    }

} // namespace atmo::meta
