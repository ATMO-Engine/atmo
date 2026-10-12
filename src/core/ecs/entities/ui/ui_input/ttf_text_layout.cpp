#include "ttf_text_layout.hpp"
#include <algorithm>
#include <cmath>
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"
#include "core/resource/resource_manager.hpp"
#include "core/text/utf8.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    TtfTextLayout::~TtfTextLayout()
    {
        if (m_text)
            TTF_DestroyText(m_text);
        if (m_font)
            TTF_CloseFont(m_font);
    }

    bool TtfTextLayout::setFont(const std::string &font_path, float point_size, float dpi_x, float dpi_y)
    {
        if (font_path != m_font_path || !m_font) {
            if (m_text) {
                TTF_DestroyText(m_text);
                m_text = nullptr;
                m_engine = nullptr;
            }
            if (m_font) {
                TTF_CloseFont(m_font);
                m_font = nullptr;
            }
            m_resource.reset();
            m_font_path = font_path;
            m_point_size = 0.0f;

            try {
                m_resource = resource::ResourceManager::GetInstance().getResource<TTF_Font>(font_path);
            } catch (const std::exception &e) {
                spdlog::error("TtfTextLayout: failed to load font '{}': {}", font_path, e.what());
            }
            if (!m_resource)
                return false;

            m_font = TTF_CopyFont(m_resource.get());
            if (!m_font) {
                spdlog::error("TtfTextLayout: TTF_CopyFont failed: {}", SDL_GetError());
                return false;
            }
            TTF_SetFontStyle(m_font, TTF_STYLE_NORMAL);

            m_text = TTF_CreateText(nullptr, m_font, m_string.data(), m_string.size());
            if (!m_text) {
                spdlog::error("TtfTextLayout: TTF_CreateText failed: {}", SDL_GetError());
                return false;
            }
            TTF_SetTextWrapWidth(m_text, m_wrap_width);
            // By default SDL_ttf collapses whitespace at line ends to zero width, which would keep the caret from
            // advancing over trailing spaces.
            TTF_SetTextWrapWhitespaceVisible(m_text, true);
        }

        if (point_size != m_point_size || dpi_x != m_dpi_x || dpi_y != m_dpi_y) {
            if (!TTF_SetFontSizeDPI(m_font, point_size, static_cast<int>(dpi_x), static_cast<int>(dpi_y)))
                spdlog::error("TtfTextLayout: TTF_SetFontSizeDPI failed: {}", SDL_GetError());
            m_point_size = point_size;
            m_dpi_x = dpi_x;
            m_dpi_y = dpi_y;
        }
        return true;
    }

    void TtfTextLayout::setWrapWidth(int width)
    {
        if (width == m_wrap_width)
            return;
        m_wrap_width = width;
        if (m_text)
            TTF_SetTextWrapWidth(m_text, width);
    }

    void TtfTextLayout::setText(std::string_view text)
    {
        if (text == m_string)
            return;
        m_string.assign(text);
        if (m_text && !TTF_SetTextString(m_text, m_string.data(), m_string.size()))
            spdlog::error("TtfTextLayout: TTF_SetTextString failed: {}", SDL_GetError());
    }

    text::CaretRect TtfTextLayout::caretRect(std::size_t offset) const
    {
        const float height = lineHeight();
        if (!m_text || m_string.empty() || offset == 0)
            return { 0.0f, 0.0f, height };

        offset = std::min(offset, m_string.size());
        TTF_SubString ss;

        if (offset < m_string.size() && TTF_GetTextSubString(m_text, static_cast<int>(offset), &ss) && ss.offset == static_cast<int>(offset) &&
            !(ss.flags & TTF_SUBSTRING_TEXT_END)) {
            return { static_cast<float>(ss.rect.x), static_cast<float>(ss.line_index) * height, height };
        }

        auto prev = text::Utf8::PrevBoundary(m_string, offset);
        if (!TTF_GetTextSubString(m_text, static_cast<int>(prev), &ss))
            return { 0.0f, 0.0f, height };

        if (m_string[prev] == '\n')
            return { 0.0f, static_cast<float>(ss.line_index + 1) * height, height };
        return { static_cast<float>(ss.rect.x + ss.rect.w), static_cast<float>(ss.line_index) * height, height };
    }

    std::size_t TtfTextLayout::offsetAt(float x, float y) const
    {
        if (!m_text || m_string.empty())
            return 0;
        if (y < 0.0f)
            return 0;

        TTF_SubString ss;
        if (!TTF_GetTextSubStringForPoint(m_text, static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)), &ss))
            return 0;

        if (ss.flags & TTF_SUBSTRING_TEXT_END)
            return m_string.size();
        if (ss.length <= 0)
            return static_cast<std::size_t>(std::max(ss.offset, 0));

        auto offset = static_cast<std::size_t>(ss.offset);
        if (m_string[offset] == '\n')
            return offset;
        if (x > static_cast<float>(ss.rect.x) + static_cast<float>(ss.rect.w) * 0.5f)
            offset += static_cast<std::size_t>(ss.length);
        return std::min(offset, m_string.size());
    }

    float TtfTextLayout::lineHeight() const
    {
        if (!m_font)
            return std::max(m_point_size, 1.0f);
        return static_cast<float>(TTF_GetFontLineSkip(m_font));
    }

    float TtfTextLayout::pageHeight() const
    {
        return m_page_height > 0.0f ? m_page_height : TextLayout::pageHeight();
    }

    std::vector<SDL_FRect> TtfTextLayout::rangeRects(std::size_t start, std::size_t end) const
    {
        std::vector<SDL_FRect> rects;
        if (!m_text || start >= end)
            return rects;

        const float height = lineHeight();
        int count = 0;
        TTF_SubString **subs = TTF_GetTextSubStringsForRange(m_text, static_cast<int>(start), static_cast<int>(end - start), &count);
        if (!subs)
            return rects;

        int current_line = -1;
        for (int i = 0; i < count; ++i) {
            const TTF_SubString *ss = subs[i];
            float x = static_cast<float>(ss->rect.x);
            float w = static_cast<float>(ss->rect.w);
            if (ss->length > 0 && m_string[static_cast<std::size_t>(ss->offset)] == '\n')
                w = std::max(w, height * 0.3f);

            if (ss->line_index == current_line && !rects.empty()) {
                auto &r = rects.back();
                float right = std::max(r.x + r.w, x + w);
                r.x = std::min(r.x, x);
                r.w = right - r.x;
            } else {
                rects.push_back({ x, static_cast<float>(ss->line_index) * height, w, height });
                current_line = ss->line_index;
            }
        }
        SDL_free(subs);
        return rects;
    }

    SDL_Point TtfTextLayout::size() const
    {
        SDL_Point p{ 0, 0 };
        if (m_text)
            TTF_GetTextSize(m_text, &p.x, &p.y);
        return p;
    }

    void TtfTextLayout::draw(TTF_TextEngine *engine, float x, float y, SDL_Color color)
    {
        if (!m_text || m_string.empty())
            return;
        if (m_engine != engine) {
            TTF_SetTextEngine(m_text, engine);
            m_engine = engine;
        }
        TTF_SetTextColor(m_text, color.r, color.g, color.b, color.a);
        TTF_DrawRendererText(m_text, x, y);
    }
} // namespace atmo::core::ecs::entities
