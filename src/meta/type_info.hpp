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
#include "glaze/json/prettify.hpp"

#include "core/resource/subresource_registry.hpp"
#include "meta/component_meta.hpp"
#include "meta/field_descriptor.hpp"
#include "meta/glaze_bridge.hpp"
#include "meta/vector_traits.hpp"

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

        // Populated when the field type is std::vector<T> (plain) or std::vector<shared_ptr<SubResource-derived>>.
        bool is_vector = false;
        bool is_subresource_vector = false;

        std::size_t (*vector_size)(const void *vec) = nullptr;
        void (*vector_clear)(void *vec) = nullptr;
        void (*vector_erase)(void *vec, std::size_t index) = nullptr;
        void *(*vector_element_ptr)(void *vec, std::size_t index) = nullptr;

        // Plain-vector element access: lets a generic "vector" widget hand each element straight into the
        // existing, unmodified WidgetRegistry via an ad hoc per-row FieldInfo using these identity get/set pairs.
        const char *element_widget = nullptr;
        std::size_t element_size = 0;
        void (*element_get)(const void *element, void *out) = nullptr;
        void (*element_set)(void *element, const void *value) = nullptr;
        void (*vector_push_default)(void *vec) = nullptr;

        // Subresource-vector specifics.
        const char *subresource_base_full_name = nullptr;
        void (*vector_push_subresource)(void *vec, std::string_view type_name) = nullptr;
        std::string_view (*subresource_element_type_name)(void *element) = nullptr;
    };

    struct TypeInfo {
        const char *name = nullptr;
        const char *category = nullptr;
        std::size_t size = 0;
        std::uint64_t flecs_id = 0;
        std::vector<FieldInfo> fields;

        void (*register_flecs)(flecs::world &) = nullptr;
        std::uint64_t (*resolve_flecs_id)(flecs::world &) = nullptr;
        glz::generic (*to_json)(const void *component) = nullptr;
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
            fi.widget = fd.widget ? fd.widget : DefaultWidget<FieldT>::value;
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

            if constexpr (IsPlainVector<FieldT>) {
                using Elem = typename IsStdVector<FieldT>::element_type;

                fi.is_vector = true;
                fi.element_widget = DefaultWidget<Elem>::value;
                fi.element_size = sizeof(Elem);

                fi.element_get = [](const void *element, void *out) { *static_cast<Elem *>(out) = *static_cast<const Elem *>(element); };
                fi.element_set = [](void *element, const void *value) { *static_cast<Elem *>(element) = *static_cast<const Elem *>(value); };

                fi.vector_size = [](const void *vec) -> std::size_t { return static_cast<const FieldT *>(vec)->size(); };
                fi.vector_clear = [](void *vec) { static_cast<FieldT *>(vec)->clear(); };
                fi.vector_erase = [](void *vec, std::size_t index) {
                    auto *v = static_cast<FieldT *>(vec);
                    if (index < v->size()) {
                        v->erase(v->begin() + static_cast<std::ptrdiff_t>(index));
                    }
                };
                fi.vector_element_ptr = [](void *vec, std::size_t index) -> void * { return &(*static_cast<FieldT *>(vec))[index]; };
                fi.vector_push_default = [](void *vec) { static_cast<FieldT *>(vec)->push_back(Elem{}); };
            } else if constexpr (IsSubResourceVector<FieldT>) {
                using Base = typename IsSharedPtr<typename IsStdVector<FieldT>::element_type>::pointee;

                fi.is_vector = true;
                fi.is_subresource_vector = true;
                fi.subresource_base_full_name = Base::FullName().data();

                fi.vector_size = [](const void *vec) -> std::size_t { return static_cast<const FieldT *>(vec)->size(); };
                fi.vector_clear = [](void *vec) { static_cast<FieldT *>(vec)->clear(); };
                fi.vector_erase = [](void *vec, std::size_t index) {
                    auto *v = static_cast<FieldT *>(vec);
                    if (index < v->size()) {
                        v->erase(v->begin() + static_cast<std::ptrdiff_t>(index));
                    }
                };
                fi.vector_element_ptr = [](void *vec, std::size_t index) -> void * { return (*static_cast<FieldT *>(vec))[index].get(); };
                fi.vector_push_subresource = [](void *vec, std::string_view type_name) {
                    auto shape = core::resource::SubResourceRegistry::Create<Base>(type_name);
                    if (shape) {
                        static_cast<FieldT *>(vec)->push_back(shape);
                    }
                };
                fi.subresource_element_type_name = [](void *element) -> std::string_view { return static_cast<Base *>(element)->getTypeName(); };
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

        ti.to_json = [](const void *component) -> glz::generic {
            auto json_str = glz::write_json(*static_cast<const T *>(component)).value_or("{}");
            return glz::read_json<glz::generic>(json_str).value_or(glz::generic{});
        };

        ti.from_json = [](void *component, std::string_view json) -> bool {
            auto err = glz::read_json(*static_cast<T *>(component), json);
            return !err;
        };

        return ti;
    }

} // namespace atmo::meta
