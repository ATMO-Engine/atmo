#include "common/utils.hpp"
#include <sstream>
#include <string>
#include <vector>

namespace atmo
{
    namespace common
    {
        std::vector<std::string> Utils::splitString(const std::string &str, char delimiter)
        {
            std::vector<std::string> tokens;
            std::istringstream stream(str);
            std::string token;

            while (std::getline(stream, token, delimiter)) {
                tokens.push_back(token);
            }
            return tokens;
        }
    } // namespace core
} // namespace atmo
