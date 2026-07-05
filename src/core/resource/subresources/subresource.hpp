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
    };
} // namespace atmo::core::resource

template <typename T>
    requires std::derived_from<T, atmo::core::resource::SubResource>
struct atmo::meta::DefaultWidget<std::vector<std::shared_ptr<T>>> {
    static constexpr const char *value = "subresource_list";
};
