#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace atmo::core::text
{
    struct LineCol {
        std::size_t line = 0;
        std::size_t column = 0; ///< Byte column within the line.

        bool operator==(const LineCol &) const = default;
    };

    /**
     * @brief UTF-8 text storage with a line index.
     *
     * All offsets are byte offsets and are snapped to codepoint boundaries. The storage is intentionally hidden behind
     * this interface so it can later be swapped for a piece table or rope (code editor) without touching callers.
     */
    class TextBuffer
    {
    public:
        TextBuffer() = default;
        explicit TextBuffer(std::string_view text);

        void setText(std::string_view text);
        void insert(std::size_t offset, std::string_view text);
        void erase(std::size_t offset, std::size_t length);

        [[nodiscard]] const std::string &text() const noexcept
        {
            return m_text;
        }
        [[nodiscard]] std::string_view slice(std::size_t start, std::size_t end) const noexcept;
        [[nodiscard]] std::size_t size() const noexcept
        {
            return m_text.size();
        }
        [[nodiscard]] bool empty() const noexcept
        {
            return m_text.empty();
        }

        [[nodiscard]] std::size_t lineCount() const noexcept
        {
            return m_line_starts.size();
        }
        /// Byte range [start, end) of a line, excluding its trailing '\n'.
        [[nodiscard]] std::pair<std::size_t, std::size_t> lineRange(std::size_t line) const noexcept;
        [[nodiscard]] std::size_t lineOf(std::size_t offset) const noexcept;
        [[nodiscard]] LineCol offsetToLineCol(std::size_t offset) const noexcept;
        [[nodiscard]] std::size_t lineColToOffset(LineCol pos) const noexcept;

        [[nodiscard]] std::size_t clamp(std::size_t offset) const noexcept;

    private:
        void rebuildLineStarts(std::size_t from_line);

        std::string m_text;
        std::vector<std::size_t> m_line_starts{ 0 };
    };
} // namespace atmo::core::text
