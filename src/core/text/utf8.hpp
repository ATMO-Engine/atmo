#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace atmo::core::text
{
    /**
     * @brief UTF-8 helpers operating on byte offsets.
     *
     * "Boundary" currently means codepoint boundary. Callers must not assume more than that so that grapheme
     * clustering can be introduced later without changing call sites.
     */
    class Utf8
    {
    public:
        static constexpr char32_t INVALID = 0xFFFD;

        static bool IsContinuation(unsigned char c) noexcept
        {
            return (c & 0xC0) == 0x80;
        }

        static std::size_t NextBoundary(std::string_view str, std::size_t offset) noexcept;
        static std::size_t PrevBoundary(std::string_view str, std::size_t offset) noexcept;
        static std::size_t SnapToBoundary(std::string_view str, std::size_t offset) noexcept;

        static char32_t Decode(std::string_view str, std::size_t offset) noexcept;
        static void Append(std::string &out, char32_t cp);
        static std::size_t CodepointCount(std::string_view str) noexcept;

        static bool IsWordChar(char32_t cp) noexcept;
        static bool IsWhitespace(char32_t cp) noexcept;

        /// Start of the word/run containing or preceding `offset` (Ctrl/Option + Left semantics).
        static std::size_t WordStart(std::string_view str, std::size_t offset) noexcept;
        /// End of the word/run containing or following `offset` (Ctrl/Option + Right semantics).
        static std::size_t WordEnd(std::string_view str, std::size_t offset) noexcept;

        /// The [start, end) range of the word under `offset`, used for double-click selection.
        static std::pair<std::size_t, std::size_t> WordAt(std::string_view str, std::size_t offset) noexcept;

    private:
        Utf8() = default;
    };
} // namespace atmo::core::text
