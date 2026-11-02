#include "canvas_view.hpp"

#include <algorithm>
#include <cmath>

namespace atmo::core
{
    namespace
    {
        using server::rendering::Affine2d;

        Affine2d CameraTransform(types::Vector2 size, const CameraView &camera)
        {
            if (!camera.active)
                return Affine2d::Identity();
            return Affine2d::Translation({ size.x * 0.5f, size.y * 0.5f }) * Affine2d::Scaling({ camera.zoom, camera.zoom }) *
                Affine2d::Translation({ -camera.position.x, -camera.position.y });
        }
    } // namespace

    std::optional<StretchMode> ParseStretchMode(std::string_view name) noexcept
    {
        if (name == "disabled")
            return StretchMode::Disabled;
        if (name == "fit")
            return StretchMode::Fit;
        if (name == "integer")
            return StretchMode::Integer;
        return std::nullopt;
    }

    std::optional<server::rendering::TextureFilter> ParseTextureFilter(std::string_view name) noexcept
    {
        if (name == "linear")
            return server::rendering::TextureFilter::Linear;
        if (name == "nearest")
            return server::rendering::TextureFilter::Nearest;
        return std::nullopt;
    }

    CanvasView ComputeCanvasView(StretchMode mode, types::Vector2 design_size, types::Vector2 viewport_size, types::Vector2 dpi_scale, const CameraView &camera)
    {
        if (mode == StretchMode::Disabled || design_size.x <= 0.0f || design_size.y <= 0.0f || viewport_size.x <= 0.0f || viewport_size.y <= 0.0f)
            return { .transform = CameraTransform(viewport_size, camera) };

        const float dpi = std::max(dpi_scale.x, 1e-3f);
        float scale = std::min(viewport_size.x / design_size.x, viewport_size.y / design_size.y);
        types::Vector2 offset{ (viewport_size.x - design_size.x * scale) * 0.5f, (viewport_size.y - design_size.y * scale) * 0.5f };

        if (mode == StretchMode::Integer) {
            const float physical = std::max(1.0f, std::floor(scale * dpi + 1e-4f));
            scale = physical / dpi;
            offset = { std::floor((viewport_size.x - design_size.x * scale) * 0.5f * dpi) / dpi,
                       std::floor((viewport_size.y - design_size.y * scale) * 0.5f * dpi) / dpi };
        }

        return {
            .transform = Affine2d::Translation(offset) * Affine2d::Scaling({ scale, scale }) * CameraTransform(design_size, camera),
            .content = server::rendering::Rect2{ offset, { design_size.x * scale, design_size.y * scale } },
        };
    }
} // namespace atmo::core
