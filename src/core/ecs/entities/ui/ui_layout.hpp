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
                float max = 0.0f;
            };

            SizingAxisType type = SizingAxisType::FIT;
            std::variant<MinMax, float> size = MinMax{};
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

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Layout::SizingAxis::MinMax> {
    static constexpr const char *name = "MinMax";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::Layout::SizingAxis::MinMax::min>("min"),
        atmo::meta::field<&atmo::core::components::Layout::SizingAxis::MinMax::max>("max"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Layout::SizingAxis> {
    static constexpr const char *name = "Size";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::Layout::SizingAxis::type>("type"),
        atmo::meta::field<&atmo::core::components::Layout::SizingAxis::size>("size").skipFlecs());
};

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Layout::Padding> {
    static constexpr const char *name = "Padding";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::Layout::Padding::left>("left"), atmo::meta::field<&atmo::core::components::Layout::Padding::right>("right"),
        atmo::meta::field<&atmo::core::components::Layout::Padding::top>("top"), atmo::meta::field<&atmo::core::components::Layout::Padding::bottom>("bottom"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Layout> {
    static constexpr const char *name = "Layout";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::Layout::width>("width"), atmo::meta::field<&atmo::core::components::Layout::height>("height"),
        atmo::meta::field<&atmo::core::components::Layout::padding>("padding"), atmo::meta::field<&atmo::core::components::Layout::child_gap>("gap"));
};
