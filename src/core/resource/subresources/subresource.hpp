#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "core/registry/hierarchic_registry.hpp"
#include "core/types.hpp"
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

        virtual std::string serialize() const = 0;
        virtual void deserialize(const std::string &data) = 0;
    };
} // namespace atmo::core::resource
