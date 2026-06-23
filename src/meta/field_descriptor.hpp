#pragma once

#include <cstddef>
#include <optional>
#include <type_traits>
#include "core/types.hpp"

namespace atmo::meta
{
    namespace detail
    {
        template <typename T> struct MemberTraits;

        template <typename Owner, typename Field> struct MemberTraits<Field Owner::*> {
            using owner_type = Owner;
            using field_type = Field;
        };
    } // namespace detail

    template <typename Owner, auto MemberPtr> struct FieldDescriptor {
        using field_type = typename detail::MemberTraits<decltype(MemberPtr)>::field_type;
        static constexpr auto member_ptr = MemberPtr;

        const char *name = nullptr;     // JSON / flecs key  (required)
        const char *label = nullptr;    // human-readable display name
        const char *tooltip = nullptr;  // editor tooltip
        const char *category = nullptr; // editor group/section
        const char *widget = nullptr;   // widget hint: "color_picker", "file_path", …
        bool read_only = false;
        bool skip_flecs = false;
        bool has_range = false;
        float range_min = 0.0f;
        float range_max = 0.0f;

        [[nodiscard]] constexpr FieldDescriptor withLabel(const char *v) const
        {
            FieldDescriptor copy = *this;
            copy.label = v;
            return copy;
        }

        [[nodiscard]] constexpr FieldDescriptor withTooltip(const char *v) const
        {
            FieldDescriptor copy = *this;
            copy.tooltip = v;
            return copy;
        }

        [[nodiscard]] constexpr FieldDescriptor withCategory(const char *v) const
        {
            FieldDescriptor copy = *this;
            copy.category = v;
            return copy;
        }

        [[nodiscard]] constexpr FieldDescriptor withWidget(const char *v) const
        {
            FieldDescriptor copy = *this;
            copy.widget = v;
            return copy;
        }

        [[nodiscard]] constexpr FieldDescriptor withRange(float mn, float mx) const
        {
            FieldDescriptor copy = *this;
            copy.has_range = true;
            copy.range_min = mn;
            copy.range_max = mx;
            return copy;
        }

        [[nodiscard]] constexpr FieldDescriptor asReadOnly() const
        {
            FieldDescriptor copy = *this;
            copy.read_only = true;
            return copy;
        }

        [[nodiscard]] constexpr FieldDescriptor skipFlecs() const
        {
            FieldDescriptor copy = *this;
            copy.skip_flecs = true;
            return copy;
        }
    };

    /**
     * @brief Factory function to create a FieldDescriptor for a given member pointer.
     *
     * @tparam MemberPtr A pointer to a non-static data member of a struct/class. Example: &MyComponent::position
     * @param name The name of the field, used as the key for serialization and in the editor. Should typically match the member name.
     * @return constexpr auto A FieldDescriptor instance with the specified name and member pointer, and deduced Owner type.
     */
    template <auto MemberPtr> [[nodiscard]] constexpr auto field(const char *name)
    {
        using Owner = typename detail::MemberTraits<decltype(MemberPtr)>::owner_type;
        return FieldDescriptor<Owner, MemberPtr>{ .name = name };
    }

    template <typename T> struct DefaultWidget {
        static constexpr const char *value = nullptr;
    };

    template <> struct DefaultWidget<int> {
        static constexpr const char *value = "int";
    };

    template <> struct DefaultWidget<float> {
        static constexpr const char *value = "float";
    };

    template <> struct DefaultWidget<core::types::Color> {
        static constexpr const char *value = "color_picker";
    };

    template <> struct DefaultWidget<core::types::Vector2> {
        static constexpr const char *value = "vector2";
    };

    template <> struct DefaultWidget<core::types::Vector2i> {
        static constexpr const char *value = "vector2i";
    };

    template <> struct DefaultWidget<core::types::Vector3> {
        static constexpr const char *value = "vector3";
    };

    template <> struct DefaultWidget<core::types::Vector3i> {
        static constexpr const char *value = "vector3i";
    };

    template <> struct DefaultWidget<core::types::Vector4> {
        static constexpr const char *value = "vector4";
    };

    template <> struct DefaultWidget<core::types::Vector4i> {
        static constexpr const char *value = "vector4i";
    };

} // namespace atmo::meta
