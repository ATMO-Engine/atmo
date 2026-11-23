#include "http_types.hpp"

#include <algorithm>
#include <cctype>

namespace atmo::mcp::http
{
    std::optional<std::string_view> FindHeader(const Headers &headers, std::string_view name)
    {
        const auto same = [name](const std::pair<std::string, std::string> &header) {
            return std::ranges::equal(
                header.first, name, [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
        };
        auto it = std::ranges::find_if(headers, same);
        if (it == headers.end())
            return std::nullopt;
        return std::string_view(it->second);
    }
} // namespace atmo::mcp::http
