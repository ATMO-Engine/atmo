#include "clay_canvas_renderer.hpp"

#include <numbers>
#include <string_view>

#include "clay_types.hpp"
#include "core/ecs/entities/ui/ui_image/ui_image.hpp"
#include "core/resource/font.hpp"
#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

namespace atmo::impl
{
    namespace
    {
        using server::rendering::Affine2d;
        using server::rendering::ColorRgba;
        using server::rendering::FontStyle;
        using server::rendering::Rect2;
        using server::rendering::RectStyle;

        core::types::Color ToColor(const Clay_Color &color)
        {
            return { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
        }

        ColorRgba ToRgba(const Clay_Color &color) noexcept
        {
            return { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
        }

        Rect2 ToRect(const Clay_BoundingBox &box) noexcept
        {
            return { { box.x, box.y }, { box.width, box.height } };
        }

        std::array<float, 4> Radii(const Clay_CornerRadius &radius) noexcept
        {
            return { radius.topLeft, radius.topRight, radius.bottomRight, radius.bottomLeft };
        }

        FontStyle StyleFrom(std::uint16_t font_id) noexcept
        {
            return { .bold = (font_id & clay_font_flags::bold) != 0, .italic = (font_id & clay_font_flags::italic) != 0 };
        }
    } // namespace

    Clay_Dimensions ClayMeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *)
    {
        const auto *font = static_cast<const core::resource::Font *>(config->userData);
        if (!font)
            return { 0.0f, 0.0f };

        const core::types::Vector2 size = server::rendering::RenderingServer::Instance().fontMeasureText(
            font->rid(), std::string_view(text.chars, static_cast<std::size_t>(text.length)), config->fontSize, StyleFrom(config->fontId));
        return { size.x, size.y };
    }

    void ClayRenderCommands(const Clay_RenderCommandArray &commands, server::rendering::CanvasCommandWriter &writer, float raster_scale)
    {
        auto &server = server::rendering::RenderingServer::Instance();
        auto &array = const_cast<Clay_RenderCommandArray &>(commands);

        for (std::int32_t i = 0; i < array.length; ++i) {
            const Clay_RenderCommand *command = Clay_RenderCommandArray_Get(&array, i);
            const Rect2 rect = ToRect(command->boundingBox);

            switch (command->commandType) {
                case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
                    {
                        const Clay_RectangleRenderData &data = command->renderData.rectangle;
                        if (data.backgroundColor.a > 0.0f)
                            writer.addRect(rect, ToColor(data.backgroundColor), { .corner_radii = Radii(data.cornerRadius) });
                        break;
                    }
                case CLAY_RENDER_COMMAND_TYPE_BORDER:
                    {
                        const Clay_BorderRenderData &data = command->renderData.border;
                        const RectStyle style{
                            .corner_radii = Radii(data.cornerRadius),
                            .border_widths = { static_cast<float>(data.width.left),
                                               static_cast<float>(data.width.top),
                                               static_cast<float>(data.width.right),
                                               static_cast<float>(data.width.bottom) },
                            .border_color = ToRgba(data.color),
                        };
                        if (data.color.a > 0.0f && !style.isPlain())
                            writer.addRect(rect, core::types::Color(0.0f, 0.0f, 0.0f, 0.0f), style);
                        break;
                    }
                case CLAY_RENDER_COMMAND_TYPE_TEXT:
                    {
                        const Clay_TextRenderData &data = command->renderData.text;
                        const auto *font = static_cast<const core::resource::Font *>(command->userData);
                        if (!font || data.stringContents.length <= 0)
                            break;

                        const FontStyle style = StyleFrom(data.fontId);
                        const std::string_view text(data.stringContents.chars, static_cast<std::size_t>(data.stringContents.length));
                        const float natural = server.fontMeasureText(font->rid(), {}, static_cast<float>(data.fontSize), style).y;
                        writer.addText(
                            font->rid(),
                            text,
                            {
                                .position = { rect.position.x, rect.position.y + ((rect.size.y - natural) * 0.5f) },
                                .size = static_cast<float>(data.fontSize),
                                .style = style,
                                .color = ToRgba(data.textColor),
                                .raster_scale = raster_scale,
                            });
                        break;
                    }
                case CLAY_RENDER_COMMAND_TYPE_IMAGE:
                    {
                        const Clay_ImageRenderData &data = command->renderData.image;
                        const auto *texture = static_cast<const server::rendering::TextureRid *>(data.imageData);
                        auto *image = static_cast<core::components::UIImage *>(command->userData);
                        if (image)
                            image->rendered_size = { rect.size.x, rect.size.y };
                        if (!texture || !texture->isValid())
                            break;

                        const server::rendering::TextureRectOptions options{
                            .modulate = data.backgroundColor.a > 0.0f ? ToRgba(data.backgroundColor) : server::rendering::white,
                            .filter = image ? image->filter : server::rendering::TextureFilter::Linear,
                        };
                        const float rotation = image ? image->rotation : 0.0f;
                        if (rotation != 0.0f) {
                            const core::types::Vector2 center{ rect.position.x + (rect.size.x * 0.5f), rect.position.y + (rect.size.y * 0.5f) };
                            writer.setTransform(
                                Affine2d::Translation(center) * Affine2d::Rotation(rotation * std::numbers::pi_v<float> / 180.0f) *
                                Affine2d::Translation({ -center.x, -center.y }));
                            writer.addTextureRect(rect, *texture, options);
                            writer.setTransform(Affine2d::Identity());
                        } else {
                            writer.addTextureRect(rect, *texture, options);
                        }
                        break;
                    }
                case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
                    writer.pushClip(rect);
                    break;
                case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
                    writer.popClip();
                    break;
                case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
                    if (auto *custom = static_cast<ClayCustomRenderable *>(command->renderData.custom.customData))
                        custom->render(writer, rect, raster_scale);
                    break;
                default:
                    spdlog::debug("Unknown Clay render command type {}", static_cast<int>(command->commandType));
                    break;
            }
        }
    }
} // namespace atmo::impl
