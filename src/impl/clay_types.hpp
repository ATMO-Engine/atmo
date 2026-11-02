#pragma once

#include <cstdint>

#include "server/rendering/canvas_commands.hpp"
#include "server/rendering/types.hpp"

/**
 * @brief Pass a pointer to an implementation as `.custom.customData` of a Clay element to draw it yourself.
 *
 * `rect` is the element's bounding box in logical units, the same space as the writer's commands.
 * `raster_scale` is the number of physical pixels per logical unit, used to rasterize text and snap lines.
 */
struct ClayCustomRenderable {
    virtual ~ClayCustomRenderable() = default;
    virtual void render(atmo::server::rendering::CanvasCommandWriter &writer, const atmo::server::rendering::Rect2 &rect, float raster_scale) = 0;
};

/**
 * @brief Bits stored in `Clay_TextElementConfig::fontId`. `userData` points to the `atmo::core::resource::Font`.
 */
namespace clay_font_flags
{
    inline constexpr std::uint16_t bold = 1;
    inline constexpr std::uint16_t italic = 2;
} // namespace clay_font_flags
