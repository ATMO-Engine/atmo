#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "core/types.hpp"
#include "server/rendering/types.hpp"

namespace atmo::core
{
    enum class StretchMode : std::uint8_t {
        Disabled,
        Fit,
        Integer,
    };

    [[nodiscard]] std::optional<StretchMode> ParseStretchMode(std::string_view name) noexcept;
    [[nodiscard]] std::optional<server::rendering::TextureFilter> ParseTextureFilter(std::string_view name) noexcept;

    struct CameraView {
        bool active = false;
        float zoom = 1.0f;
        types::Vector2 position{ 0.0f, 0.0f };
    };

    struct CanvasView {
        server::rendering::Affine2d transform;
        std::optional<server::rendering::Rect2> content;
    };

    [[nodiscard]] CanvasView
    ComputeCanvasView(StretchMode mode, types::Vector2 design_size, types::Vector2 viewport_size, types::Vector2 dpi_scale, const CameraView &camera);
} // namespace atmo::core
