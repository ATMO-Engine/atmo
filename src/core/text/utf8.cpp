#include "utf8.hpp"
#include <algorithm>

namespace atmo::core::text
{
    namespace
    {
        enum class CharClass {
            Word,
            Punct,
            Space,
            Newline
        };

        CharClass Classify(char32_t cp) noexcept
        {
            if (cp == U'\n')
                return CharClass::Newline;
            if (Utf8::IsWhitespace(cp))
                return CharClass::Space;
            if (Utf8::IsWordChar(cp))
                return CharClass::Word;
            return CharClass::Punct;
        }
    } // namespace

    std::size_t Utf8::NextBoundary(std::string_view str, std::size_t offset) noexcept
    {
        if (offset >= str.size())
            return str.size();
        ++offset;
        while (offset < str.size() && IsContinuation(static_cast<unsigned char>(str[offset]))) ++offset;
        return offset;
    }

    std::size_t Utf8::PrevBoundary(std::string_view str, std::size_t offset) noexcept
    {
        if (offset == 0)
            return 0;
        offset = std::min(offset, str.size());
        --offset;
        while (offset > 0 && IsContinuation(static_cast<unsigned char>(str[offset]))) --offset;
        return offset;
    }

    std::size_t Utf8::SnapToBoundary(std::string_view str, std::size_t offset) noexcept
    {
        if (offset >= str.size())
            return str.size();
        while (offset > 0 && IsContinuation(static_cast<unsigned char>(str[offset]))) --offset;
        return offset;
    }

    char32_t Utf8::Decode(std::string_view str, std::size_t offset) noexcept
    {
        if (offset >= str.size())
            return 0;

        auto c0 = static_cast<unsigned char>(str[offset]);
        std::size_t len;
        char32_t cp;

        if (c0 < 0x80)
            return c0;
        if ((c0 & 0xE0) == 0xC0) {
            len = 2;
            cp = c0 & 0x1F;
        } else if ((c0 & 0xF0) == 0xE0) {
            len = 3;
            cp = c0 & 0x0F;
        } else if ((c0 & 0xF8) == 0xF0) {
            len = 4;
            cp = c0 & 0x07;
        } else {
            return INVALID;
        }

        if (offset + len > str.size())
            return INVALID;

        for (std::size_t i = 1; i < len; ++i) {
            auto c = static_cast<unsigned char>(str[offset + i]);
            if (!IsContinuation(c))
                return INVALID;
            cp = (cp << 6) | (c & 0x3F);
        }
        return cp;
    }

    void Utf8::Append(std::string &out, char32_t cp)
    {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    std::size_t Utf8::CodepointCount(std::string_view str) noexcept
    {
        std::size_t count = 0;
        for (char c : str)
            if (!IsContinuation(static_cast<unsigned char>(c)))
                ++count;
        return count;
    }

    bool Utf8::IsWordChar(char32_t cp) noexcept
    {
        if (cp >= 0x80)
            return !IsWhitespace(cp);
        return (cp >= U'a' && cp <= U'z') || (cp >= U'A' && cp <= U'Z') || (cp >= U'0' && cp <= U'9') || cp == U'_';
    }

    bool Utf8::IsWhitespace(char32_t cp) noexcept
    {
        return cp == U' ' || cp == U'\t' || cp == U'\n' || cp == U'\r' || cp == U'\v' || cp == U'\f' || cp == 0x00A0 || cp == 0x3000;
    }

    std::size_t Utf8::WordStart(std::string_view str, std::size_t offset) noexcept
    {
        offset = SnapToBoundary(str, offset);

        while (offset > 0) {
            auto prev = PrevBoundary(str, offset);
            if (Classify(Decode(str, prev)) != CharClass::Space)
                break;
            offset = prev;
        }
        if (offset == 0)
            return 0;

        auto prev = PrevBoundary(str, offset);
        auto cls = Classify(Decode(str, prev));
        if (cls == CharClass::Newline)
            return prev;

        while (offset > 0) {
            prev = PrevBoundary(str, offset);
            if (Classify(Decode(str, prev)) != cls)
                break;
            offset = prev;
        }
        return offset;
    }

    std::size_t Utf8::WordEnd(std::string_view str, std::size_t offset) noexcept
    {
        offset = SnapToBoundary(str, offset);

        while (offset < str.size() && Classify(Decode(str, offset)) == CharClass::Space) offset = NextBoundary(str, offset);
        if (offset >= str.size())
            return str.size();

        auto cls = Classify(Decode(str, offset));
        if (cls == CharClass::Newline)
            return NextBoundary(str, offset);

        while (offset < str.size() && Classify(Decode(str, offset)) == cls) offset = NextBoundary(str, offset);
        return offset;
    }

    std::pair<std::size_t, std::size_t> Utf8::WordAt(std::string_view str, std::size_t offset) noexcept
    {
        if (str.empty())
            return { 0, 0 };

        offset = SnapToBoundary(str, offset);
        if (offset >= str.size() || (Decode(str, offset) == U'\n' && offset > 0))
            offset = PrevBoundary(str, offset);

        auto cls = Classify(Decode(str, offset));
        if (cls == CharClass::Newline)
            return { offset, offset };

        std::size_t start = offset;
        while (start > 0) {
            auto prev = PrevBoundary(str, start);
            if (Classify(Decode(str, prev)) != cls)
                break;
            start = prev;
        }
        std::size_t end = offset;
        while (end < str.size() && Classify(Decode(str, end)) == cls) end = NextBoundary(str, end);
        return { start, end };
    }
} // namespace atmo::core::text
