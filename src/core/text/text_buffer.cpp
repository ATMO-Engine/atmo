#include "text_buffer.hpp"
#include <algorithm>
#include "core/text/utf8.hpp"

namespace atmo::core::text
{
    TextBuffer::TextBuffer(std::string_view text)
    {
        setText(text);
    }

    void TextBuffer::setText(std::string_view text)
    {
        m_text.assign(text);
        rebuildLineStarts(0);
    }

    void TextBuffer::insert(std::size_t offset, std::string_view text)
    {
        if (text.empty())
            return;
        offset = clamp(offset);
        auto line = lineOf(offset);
        m_text.insert(offset, text);
        rebuildLineStarts(line);
    }

    void TextBuffer::erase(std::size_t offset, std::size_t length)
    {
        offset = clamp(offset);
        auto end = clamp(offset + std::min(length, m_text.size() - offset));
        if (end <= offset)
            return;
        auto line = lineOf(offset);
        m_text.erase(offset, end - offset);
        rebuildLineStarts(line);
    }

    std::string_view TextBuffer::slice(std::size_t start, std::size_t end) const noexcept
    {
        start = std::min(start, m_text.size());
        end = std::clamp(end, start, m_text.size());
        return std::string_view(m_text).substr(start, end - start);
    }

    std::pair<std::size_t, std::size_t> TextBuffer::lineRange(std::size_t line) const noexcept
    {
        line = std::min(line, m_line_starts.size() - 1);
        auto start = m_line_starts[line];
        auto end = line + 1 < m_line_starts.size() ? m_line_starts[line + 1] - 1 : m_text.size();
        return { start, end };
    }

    std::size_t TextBuffer::lineOf(std::size_t offset) const noexcept
    {
        auto it = std::upper_bound(m_line_starts.begin(), m_line_starts.end(), offset);
        return static_cast<std::size_t>(std::distance(m_line_starts.begin(), it)) - 1;
    }

    LineCol TextBuffer::offsetToLineCol(std::size_t offset) const noexcept
    {
        offset = clamp(offset);
        auto line = lineOf(offset);
        return { line, offset - m_line_starts[line] };
    }

    std::size_t TextBuffer::lineColToOffset(LineCol pos) const noexcept
    {
        auto [start, end] = lineRange(pos.line);
        return clamp(std::min(start + pos.column, end));
    }

    std::size_t TextBuffer::clamp(std::size_t offset) const noexcept
    {
        return Utf8::SnapToBoundary(m_text, offset);
    }

    void TextBuffer::rebuildLineStarts(std::size_t from_line)
    {
        from_line = std::min(from_line, m_line_starts.size() - 1);
        m_line_starts.resize(from_line + 1);
        for (auto i = m_line_starts[from_line]; i < m_text.size(); ++i)
            if (m_text[i] == '\n')
                m_line_starts.push_back(i + 1);
    }
} // namespace atmo::core::text
