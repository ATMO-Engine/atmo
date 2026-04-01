#pragma once

#include <cstdint>
#include <variant>
#include "core/types.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct Layout {
        enum class Direction {
            Horizontal,
            Vertical
        };

        struct SizingAxis {
            enum class SizingAxisType {
                FIT,
                GROW,
                FIXED,
                PERCENT
            };

            struct MinMax {
                float min = 0.0f;
                float max = std::numeric_limits<float>::max();
            };

            SizingAxisType type = SizingAxisType::FIT;
            std::variant<MinMax, float> size;
        };

        struct Padding {
            std::uint16_t left = 0;
            std::uint16_t right = 0;
            std::uint16_t top = 0;
            std::uint16_t bottom = 0;
        };

        Direction direction = Direction::Horizontal;
        SizingAxis width;
        SizingAxis height;

        Padding padding;

        std::uint16_t child_gap = 0;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Layout> {
    static constexpr const char *name = "Layout";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::Layout::width>("width"), atmo::meta::field<&atmo::core::components::Layout::height>("height"),
        atmo::meta::field<&atmo::core::components::Layout::padding>("padding"), atmo::meta::field<&atmo::core::components::Layout::child_gap>("gap"));
};
