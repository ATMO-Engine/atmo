#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>

namespace atmo::core::text
{
    /**
     * @brief A caret with an optional selection. `head` is where the caret is drawn, `anchor` is the fixed end.
     */
    struct Selection {
        std::size_t anchor = 0;
        std::size_t head = 0;
        /// Sticky horizontal position used for vertical caret moves, in layout units.
        std::optional<float> preferred_x;

        static Selection Caret(std::size_t offset) noexcept
        {
            return { offset, offset, std::nullopt };
        }

        [[nodiscard]] std::size_t min() const noexcept
        {
            return std::min(anchor, head);
        }
        [[nodiscard]] std::size_t max() const noexcept
        {
            return std::max(anchor, head);
        }
        [[nodiscard]] bool empty() const noexcept
        {
            return anchor == head;
        }

        bool operator==(const Selection &other) const noexcept
        {
            return anchor == other.anchor && head == other.head;
        }
    };
} // namespace atmo::core::text
