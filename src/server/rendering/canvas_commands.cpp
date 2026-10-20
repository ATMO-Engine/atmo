#include "canvas_commands.hpp"

#include "server/rendering/rendering_server.hpp"
#include "server/rendering/text/font_system.hpp"

namespace atmo::server::rendering
{
    namespace
    {
        ColorRgba ToRgba(const core::types::Color &color) noexcept
        {
            return { color.r, color.g, color.b, color.a };
        }
    } // namespace

    void CanvasCommandWriter::addRect(const Rect2 &rect, const core::types::Color &color, const RectStyle &style)
    {
        m_commands.emplace_back(CanvasRectCommand{ .rect = rect, .color = ToRgba(color), .style = style });
    }

    void CanvasCommandWriter::addTextureRect(const Rect2 &rect, TextureRid texture, const TextureRectOptions &options)
    {
        m_commands.emplace_back(CanvasTextureRectCommand{ .rect = rect, .texture = texture, .options = options });
    }

    void CanvasCommandWriter::addPolygon(std::span<const core::types::Vector2> points, std::span<const core::types::Color> colors)
    {
        CanvasPolygonCommand command{ .points = { points.begin(), points.end() } };
        command.colors.reserve(colors.size());
        for (const core::types::Color &color : colors) command.colors.push_back(ToRgba(color));
        m_commands.emplace_back(std::move(command));
    }

    void CanvasCommandWriter::addPolyline(std::span<const core::types::Vector2> points, const core::types::Color &color, float width, bool closed)
    {
        m_commands.emplace_back(CanvasPolylineCommand{ .points = { points.begin(), points.end() }, .color = ToRgba(color), .width = width, .closed = closed });
    }

    void CanvasCommandWriter::addText(FontRid font, std::string_view utf8, const TextOptions &options)
    {
        RenderingServer &server = RenderingServer::Instance();
        if (server.isInitialized())
            server.fonts().appendText(m_commands, font, utf8, options);
    }

    void CanvasCommandWriter::addTtfText(TTF_Text *text, core::types::Vector2 position, const core::types::Color &color, float raster_scale)
    {
        RenderingServer &server = RenderingServer::Instance();
        if (server.isInitialized())
            server.fonts().appendTtfText(m_commands, text, position, ToRgba(color), raster_scale);
    }

    void CanvasCommandWriter::setTransform(const Affine2d &transform)
    {
        m_commands.emplace_back(CanvasTransformCommand{ .transform = transform });
    }

    void CanvasCommandWriter::pushClip(const Rect2 &clip)
    {
        m_commands.emplace_back(CanvasClipPushCommand{ .rect = clip });
    }

    void CanvasCommandWriter::popClip()
    {
        m_commands.emplace_back(CanvasClipPopCommand{});
    }

    void CanvasCommandWriter::clear() noexcept
    {
        m_commands.clear();
    }

    bool CanvasCommandWriter::empty() const noexcept
    {
        return m_commands.empty();
    }

    std::size_t CanvasCommandWriter::size() const noexcept
    {
        return m_commands.size();
    }

    const std::vector<CanvasCommand> &CanvasCommandWriter::commands() const noexcept
    {
        return m_commands;
    }

    std::vector<CanvasCommand> CanvasCommandWriter::take() noexcept
    {
        return std::exchange(m_commands, {});
    }
} // namespace atmo::server::rendering
