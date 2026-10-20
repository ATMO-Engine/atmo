#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#include "core/types.hpp"
#include "server/rendering/rid.hpp"
#include "server/rendering/types.hpp"

struct TTF_Text;

namespace atmo::server::rendering
{
    struct CanvasRectCommand {
        Rect2 rect;
        ColorRgba color = white;
        RectStyle style;
    };

    struct CanvasTextureRectCommand {
        Rect2 rect;
        TextureRid texture;
        TextureRectOptions options;
    };

    struct GlyphQuad {
        Rect2 rect;
        std::array<float, 4> uv{};
    };

    struct CanvasGlyphRunCommand {
        TextureRid texture;
        ColorRgba color = white;
        std::vector<GlyphQuad> quads;
    };

    struct CanvasPolygonCommand {
        std::vector<core::types::Vector2> points;
        std::vector<ColorRgba> colors;
    };

    struct CanvasPolylineCommand {
        std::vector<core::types::Vector2> points;
        ColorRgba color = white;
        float width = 1.0f;
        bool closed = false;
    };

    struct CanvasTransformCommand {
        Affine2d transform;
    };

    struct CanvasClipPushCommand {
        Rect2 rect;
    };

    struct CanvasClipPopCommand {
    };

    using CanvasCommand = std::variant<
        CanvasRectCommand, CanvasTextureRectCommand, CanvasGlyphRunCommand, CanvasPolygonCommand, CanvasPolylineCommand, CanvasTransformCommand,
        CanvasClipPushCommand, CanvasClipPopCommand>;

    class CanvasCommandWriter
    {
    public:
        void addRect(const Rect2 &rect, const core::types::Color &color, const RectStyle &style = {});
        void addTextureRect(const Rect2 &rect, TextureRid texture, const TextureRectOptions &options = {});
        void addPolygon(std::span<const core::types::Vector2> points, std::span<const core::types::Color> colors);
        void addPolyline(std::span<const core::types::Vector2> points, const core::types::Color &color, float width = 1.0f, bool closed = false);
        void addText(FontRid font, std::string_view utf8, const TextOptions &options);
        void addTtfText(TTF_Text *text, core::types::Vector2 position, const core::types::Color &color, float raster_scale = 1.0f);
        void setTransform(const Affine2d &transform);
        void pushClip(const Rect2 &clip);
        void popClip();

        void clear() noexcept;
        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] const std::vector<CanvasCommand> &commands() const noexcept;
        [[nodiscard]] std::vector<CanvasCommand> take() noexcept;

    private:
        std::vector<CanvasCommand> m_commands;
    };
} // namespace atmo::server::rendering
