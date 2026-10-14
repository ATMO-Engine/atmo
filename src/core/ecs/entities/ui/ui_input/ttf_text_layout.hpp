#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "SDL3/SDL_rect.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "core/text/text_layout.hpp"

namespace atmo::core::ecs::entities
{
    /**
     * @brief `TextLayout` backed by an SDL_ttf `TTF_Text`. Units are pixels.
     *
     * Owns a private copy of the font so its size/DPI is not shared with labels using the same font resource.
     */
    class TtfTextLayout : public text::TextLayout
    {
    public:
        TtfTextLayout() = default;
        ~TtfTextLayout() override;

        TtfTextLayout(const TtfTextLayout &) = delete;
        TtfTextLayout &operator=(const TtfTextLayout &) = delete;

        /// (Re)configures the font. Cheap when nothing changed. Returns false when the font cannot be loaded.
        bool setFont(const std::string &font_path, float point_size, float dpi_x, float dpi_y);
        /// Wrap width in pixels, 0 disables wrapping (newlines still break lines).
        void setWrapWidth(int width);

        void setText(std::string_view text) override;
        [[nodiscard]] text::CaretRect caretRect(std::size_t offset) const override;
        [[nodiscard]] std::size_t offsetAt(float x, float y) const override;
        [[nodiscard]] float lineHeight() const override;

        void setPageHeight(float height) noexcept
        {
            m_page_height = height;
        }
        [[nodiscard]] float pageHeight() const override;

        /// Rectangles covering [start, end), merged per line.
        [[nodiscard]] std::vector<SDL_FRect> rangeRects(std::size_t start, std::size_t end) const;
        [[nodiscard]] SDL_Point size() const;

        /// Draws the text at (x, y) in pixels. The renderer text engine is bound on first use.
        void draw(TTF_TextEngine *engine, float x, float y, SDL_Color color);

        [[nodiscard]] bool valid() const noexcept
        {
            return m_text != nullptr;
        }
        [[nodiscard]] const std::string &text() const noexcept
        {
            return m_string;
        }

    private:
        std::shared_ptr<TTF_Font> m_resource;
        TTF_Font *m_font = nullptr;
        TTF_Text *m_text = nullptr;
        TTF_TextEngine *m_engine = nullptr;

        std::string m_font_path;
        float m_point_size = 0.0f;
        float m_dpi_x = 0.0f;
        float m_dpi_y = 0.0f;
        int m_wrap_width = 0;
        float m_page_height = 0.0f;
        std::string m_string;
    };
} // namespace atmo::core::ecs::entities
