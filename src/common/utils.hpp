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

                static std::vector<std::string> splitString(const std::string &str, char delimiter);
            private:
                Utils() = default;

            };
    } // namespace core
} // namespace atmo
