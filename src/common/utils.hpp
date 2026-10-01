#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "core/types.hpp"

namespace atmo
{
    namespace common
    {
        class Utils
        {
        public:
            ~Utils() = default;

            static std::vector<std::string> SplitString(const std::string &str, char delimiter);
            static bool GlobMatch(std::string_view pattern, std::string_view str);

            /**
             * @brief Remove the last code point of a UTF-8 encoded string
             *
             * Removes the whole trailing code point, so a multi-byte character is dropped
             * instead of being truncated into an invalid sequence. Input that is not valid UTF-8
             * is still shortened rather than left untouched: the walk stops after the longest
             * sequence UTF-8 allows, and a string that is nothing but continuation bytes has no
             * lead byte to remove, so the counted bytes are dropped on their own.
             *
             * @param str String to modify, left unchanged when empty
             */
            static void PopBackUtf8(std::string &str);

        private:
            Utils() = default;
        };
    } // namespace common
} // namespace atmo
