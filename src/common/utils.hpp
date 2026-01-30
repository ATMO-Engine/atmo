#pragma once

#include <string>
#include <vector>

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

        private:
            Utils() = default;
        };
    } // namespace common
} // namespace atmo
