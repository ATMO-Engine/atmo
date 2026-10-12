#pragma once

#include <cstddef>
#include <string_view>

namespace atmo::core::text
{
    struct CaretRect {
        float x = 0.0f;
        float y = 0.0f;
        float height = 0.0f;
    };

    /**
     * @brief Geometry of laid-out text, used for hit-testing and caret placement.
     *
     * Coordinates are relative to the top-left of the text and expressed in the layout's own units (pixels for the
     * SDL_ttf implementation). The editor calls `setText` before every geometric query so implementations must make
     * that cheap when the text did not change.
     */
    class TextLayout
    {
    public:
        virtual ~TextLayout() = default;

        virtual void setText(std::string_view text) = 0;
        [[nodiscard]] virtual CaretRect caretRect(std::size_t offset) const = 0;
        [[nodiscard]] virtual std::size_t offsetAt(float x, float y) const = 0;
        [[nodiscard]] virtual float lineHeight() const = 0;

        /// Height scrolled by PageUp/PageDown.
        [[nodiscard]] virtual float pageHeight() const
        {
            return lineHeight() * 10.0f;
        }
    };
} // namespace atmo::core::text
