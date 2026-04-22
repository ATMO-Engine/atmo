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
