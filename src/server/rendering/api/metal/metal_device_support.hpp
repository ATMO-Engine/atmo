#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace atmo::server::rendering::metal
{
    [[nodiscard]] std::optional<std::string> DeviceSupportError(std::string_view device_name);
} // namespace atmo::server::rendering::metal
