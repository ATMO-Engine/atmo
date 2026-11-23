#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "spdlog/common.h"

namespace atmo::mcp
{
    class LogBuffer
    {
    public:
        struct Entry {
            std::uint64_t seq = 0;
            std::string time;
            std::string level;
            std::string message;
        };

        static constexpr std::size_t capacity = 2000;

        static void Install();
        [[nodiscard]] static std::vector<Entry> Read(std::uint64_t since, spdlog::level::level_enum min_level, std::size_t limit);
        [[nodiscard]] static std::uint64_t LastSequence();
    };
} // namespace atmo::mcp
