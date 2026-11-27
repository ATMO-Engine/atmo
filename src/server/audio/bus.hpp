#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace atmo::server::audio
{
    /**
     * @brief Group of voices sharing a volume. Every bus is mixed into Master.
     */
    enum class Bus : std::uint8_t {
        Master,
        Music,
        Sfx
    };

    inline constexpr std::array<std::string_view, 3> bus_names{ "master", "music", "sfx" };

    [[nodiscard]] constexpr std::size_t BusIndex(Bus bus) noexcept
    {
        return static_cast<std::size_t>(bus);
    }

    [[nodiscard]] constexpr std::string_view BusName(Bus bus) noexcept
    {
        return BusIndex(bus) < bus_names.size() ? bus_names[BusIndex(bus)] : std::string_view{};
    }

    [[nodiscard]] constexpr std::optional<Bus> ParseBus(std::string_view name) noexcept
    {
        const auto it = std::ranges::find(bus_names, name);
        if (it == bus_names.end())
            return std::nullopt;
        return static_cast<Bus>(it - bus_names.begin());
    }
} // namespace atmo::server::audio
