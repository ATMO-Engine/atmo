#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace atmo::mcp::http
{
    using Headers = std::vector<std::pair<std::string, std::string>>;

    std::optional<std::string_view> FindHeader(const Headers &headers, std::string_view name);
} // namespace atmo::mcp::http
