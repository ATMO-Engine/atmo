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
                Utils() = default;
                ~Utils() = default;

                static std::vector<std::string> splitString(const std::string &str, char delimiter);
            };
    } // namespace core
} // namespace atmo
