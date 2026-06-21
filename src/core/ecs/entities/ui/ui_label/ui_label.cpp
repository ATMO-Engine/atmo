#include "ui_label.hpp"
#include "SDL3/SDL_error.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "core/types.hpp"
#include "locale/locale_manager.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UILabel::RegisterSystems(flecs::world *world)
    {
        world->system<components::UILabel>("UILabel_sync").kind(flecs::PreUpdate).each([](flecs::entity /*e*/, components::UILabel &label) {
            if (!label.m_render_cache) {
                label.m_render_cache = std::make_unique<components::UILabel::TextRenderCache>();
                label.m_prev_font_path = "";
                label.m_prev_font_size = 0;
                label.m_prev_font_style = -1;
            }

            bool font_changed = (label.font_path != label.m_prev_font_path);
            if (font_changed) {
                if (label.m_render_cache->ttf_text) {
                    TTF_DestroyText(label.m_render_cache->ttf_text);
                    label.m_render_cache->ttf_text = nullptr;
                    label.m_render_cache->engine_bound = false;
                }
                label.m_res = nullptr;

                if (!label.font_path.empty()) {
                    label.m_res = resource::ResourceManager::GetInstance().getResource<TTF_Font>(label.font_path);
                    auto font_ptr = label.m_res->get();
                    if (font_ptr) {
                        label.m_render_cache->ttf_text = TTF_CreateText(nullptr, font_ptr.get(), label.text.c_str(), label.text.size());
                        label.m_render_cache->dirty = true;
                    }
                }

                label.m_prev_font_path = label.font_path;
                label.m_prev_text = label.text;
                label.m_prev_font_size = 0;
                label.m_prev_font_style = -1;
            }

            if (!label.m_render_cache->ttf_text)
                return;

            if (label.text != label.m_prev_text) {
                if (!TTF_SetTextString(label.m_render_cache->ttf_text, label.text.c_str(), label.text.size()))
                    spdlog::error("UILabel_sync: TTF_SetTextString failed: {}", SDL_GetError());
                label.m_prev_text = label.text;
                label.m_render_cache->dirty = true;
            }

            if (label.font_size != label.m_prev_font_size) {
                TTF_Font *font = TTF_GetTextFont(label.m_render_cache->ttf_text);
                if (font)
                    TTF_SetFontSize(font, label.font_size);
                label.m_prev_font_size = label.font_size;
                label.m_render_cache->dirty = true;
            }

            int desired_style = TTF_STYLE_NORMAL;
            if (label.font_bold)
                desired_style |= TTF_STYLE_BOLD;
            if (label.font_italic)
                desired_style |= TTF_STYLE_ITALIC;

            if (desired_style != label.m_prev_font_style) {
                TTF_Font *font = TTF_GetTextFont(label.m_render_cache->ttf_text);
                if (font)
                    TTF_SetFontStyle(font, desired_style);
                label.m_prev_font_style = desired_style;
                label.m_render_cache->dirty = true;
            }
        });

        world->observer<components::UILabel>("UILabel_remove").event(flecs::OnRemove).each([](flecs::entity /*e*/, components::UILabel &label) {
            if (label.m_render_cache) {
                if (label.m_render_cache->texture) {
                    SDL_DestroyTexture(label.m_render_cache->texture);
                    label.m_render_cache->texture = nullptr;
                }
                if (label.m_render_cache->ttf_text) {
                    TTF_DestroyText(label.m_render_cache->ttf_text);
                    label.m_render_cache->ttf_text = nullptr;
                }
            }
            label.m_res = nullptr;
        });
    }

    void UILabel::initialize()
    {
        UI::initialize();

        components::UILabel comp{};
        comp.m_render_cache = std::make_unique<components::UILabel::TextRenderCache>();
        setComponent<components::UILabel>(std::move(comp));
    }

    void UILabel::setFontPath(const std::string &path)
    {
        getComponentMutable<components::UILabel>().font_path = path;
    }

    std::string_view UILabel::getFontPath() const noexcept
    {
        return getComponent<components::UILabel>().font_path;
    }

    void UILabel::setText(const std::string &text)
    {
        getComponentMutable<components::UILabel>().text = locale::LocaleManager::GetTranslation(text);
    }

    std::string_view UILabel::getText() const noexcept
    {
        return getComponent<components::UILabel>().text;
    }

    void UILabel::setFontSize(std::uint16_t font_size)
    {
        getComponentMutable<components::UILabel>().font_size = font_size;
    }

    std::uint16_t UILabel::getFontSize() const noexcept
    {
        return getComponent<components::UILabel>().font_size;
    }

    void UILabel::setFontBold(bool bold)
    {
        getComponentMutable<components::UILabel>().font_bold = bold;
    }

    void UILabel::setFontItalic(bool italic)
    {
        getComponentMutable<components::UILabel>().font_italic = italic;
    }

    void UILabel::setLetterSpacing(std::uint16_t spacing)
    {
        getComponentMutable<components::UILabel>().letter_spacing = spacing;
    }

    std::uint16_t UILabel::getLetterSpacing() const noexcept
    {
        return getComponent<components::UILabel>().letter_spacing;
    }

    void UILabel::setLineHeight(std::uint16_t height)
    {
        getComponentMutable<components::UILabel>().line_height = height;
    }

    std::uint16_t UILabel::getLineHeight() const noexcept
    {
        return getComponent<components::UILabel>().line_height;
    }

    void UILabel::setWrapMode(components::UILabel::WrapMode mode)
    {
        getComponentMutable<components::UILabel>().wrap_mode = mode;
    }

    components::UILabel::WrapMode UILabel::getWrapMode() const noexcept
    {
        return getComponent<components::UILabel>().wrap_mode;
    }

    void UILabel::setTextAlignment(components::UILabel::TextAlignment alignment)
    {
        getComponentMutable<components::UILabel>().text_alignment = alignment;
    }

    components::UILabel::TextAlignment UILabel::getTextAlignment() const noexcept
    {
        return getComponent<components::UILabel>().text_alignment;
    }

    void UILabel::draw(ClaySdL3RendererData *data)
    {
        const auto &ui = getComponent<components::UI>();
        const auto &label = getComponent<components::UILabel>();

        if (!label.m_render_cache || !label.m_render_cache->ttf_text)
            return;

        if (!label.m_render_cache->engine_bound) {
            TTF_SetTextEngine(label.m_render_cache->ttf_text, data->text_engine);
            label.m_render_cache->engine_bound = true;
        }

        Clay_String str = Clay_String{ .isStaticallyAllocated = false, .length = static_cast<int32_t>(label.text.size()), .chars = label.text.data() };
        CLAY_TEXT(
            str,
            CLAY_TEXT_CONFIG(
                { .userData = static_cast<void *>(label.m_render_cache.get()),
                  .textColor = (types::Color::WHITE * ui.modulate).toFloat<Clay_Color>(255),
                  .fontSize = static_cast<uint16_t>(label.font_size),
                  .letterSpacing = label.letter_spacing,
                  .lineHeight = label.line_height,
                  .wrapMode = static_cast<Clay_TextElementConfigWrapMode>(label.wrap_mode),
                  .textAlignment = static_cast<Clay_TextAlignment>(label.text_alignment) }));
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UILabel);
ATMO_REGISTER_COMPONENT(atmo::core::components::UILabel)
