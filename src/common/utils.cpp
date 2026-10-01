#include "common/utils.hpp"
#include <sstream>
#include <string>
#include <vector>

namespace atmo
{
    namespace common
    {
        std::vector<std::string> Utils::SplitString(const std::string &str, char delimiter)
        {
            std::vector<std::string> tokens;
            std::istringstream stream(str);
            std::string token;

            while (std::getline(stream, token, delimiter)) {
                tokens.push_back(token);
            }
            return tokens;
        }

        namespace
        {
            constexpr unsigned char Utf8ContinuationPrefix = 0x80;
            constexpr unsigned char Utf8ContinuationMask = 0xC0;
            // Longest UTF-8 sequence is 4 bytes: 1 lead byte plus 3 continuation bytes.
            constexpr std::size_t MaxUtf8ContinuationBytes = 3;

            constexpr bool IsUtf8ContinuationByte(char byte)
            {
                return (static_cast<unsigned char>(byte) & Utf8ContinuationMask) == Utf8ContinuationPrefix;
            }
        } // namespace

        void Utils::PopBackUtf8(std::string &str)
        {
            if (str.empty()) {
                return;
            }

            // A code point is one lead byte followed by up to MaxUtf8ContinuationBytes
            // continuation bytes (0b10xxxxxx). Count the continuation bytes that end the string,
            // then drop those plus the byte in front of them, which is where the last code point
            // starts.
            std::size_t continuations = 0;
            while (continuations < MaxUtf8ContinuationBytes && continuations < str.size() && IsUtf8ContinuationByte(str[str.size() - 1 - continuations])) {
                continuations++;
            }

            // Malformed input can be nothing but continuation bytes, in which case the byte in
            // front of them does not exist. Drop what was counted and stop, instead of computing
            // a size that would wrap around.
            const std::size_t removable = continuations < str.size() ? continuations + 1 : continuations;
            str.resize(str.size() - removable);
        }

        bool Utils::GlobMatch(std::string_view pattern, std::string_view str)
        {
            size_t p = 0, s = 0, star = std::string::npos, match = 0;

            while (s < str.size()) {
                if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == str[s])) {
                    ++p;
                    ++s;
                } else if (p < pattern.size() && pattern[p] == '*') {
                    star = p++;
                    match = s;
                } else if (star != std::string::npos) {
                    p = star + 1;
                    s = ++match;
                } else {
                    return false;
                }
            }

            while (p < pattern.size() && pattern[p] == '*') {
                ++p;
            }

            return p == pattern.size();
        }
    } // namespace common
} // namespace atmo
