#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/registry/hierarchic_registry.hpp"
#include "core/types.hpp"
#include "meta/field_descriptor.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::resource
{
    class SubResource
    {
    public:
        virtual ~SubResource() = default;

        static constexpr std::string_view FullName()
        {
            return "SubResource";
        }

        /**
         * @brief Runtime accessor for the concrete type's registry name (its `FullName()`), needed because
         * `FullName()` itself is `static constexpr` and can't be called polymorphically through a base pointer.
         */
        virtual std::string_view getTypeName() const = 0;

        virtual std::string serialize() const = 0;
        virtual void deserialize(const std::string &data) = 0;

        /**
         * @brief Hook invoked by the reflection system whenever a field is written through `FieldInfo::set`.
         * Lets subresources (e.g. `Shape2d`) mark themselves dirty so a downstream sync system can re-apply
         * the change to whatever external system owns the live representation (e.g. Box2D).
         */
        virtual void onFieldChanged() {}

        /**
         * @brief Hook invoked by the reflection system before a subresource is erased from its owning vector,
         * so it can tear down any external resources it registered (e.g. `Shape2d` destroying its `b2ShapeId`).
         */
        virtual void prepareForRemoval() {}
    };
} // namespace atmo::core::resource

template <typename T>
    requires std::derived_from<T, atmo::core::resource::SubResource>
struct atmo::meta::DefaultWidget<std::vector<std::shared_ptr<T>>> {
    static constexpr const char *value = "subresource_list";
};
