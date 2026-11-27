#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include "server/audio/effects/audio_effect.hpp"

namespace atmo::server::audio
{
    /**
     * @brief Effects a bus can have, in the order they are applied.
     */
    enum class EffectKind : std::uint8_t {
        HighPass,
        LowPass,
        Eq3Band,
        Delay,
        Reverb
    };

    inline constexpr std::array<std::string_view, 5> effect_names{ "high_pass", "low_pass", "eq", "delay", "reverb" };

    inline constexpr std::size_t max_effect_parameters = 4;

    struct EffectValue {
        std::string_view name;
        float value;
    };

    [[nodiscard]] constexpr std::size_t EffectIndex(EffectKind kind) noexcept
    {
        return static_cast<std::size_t>(kind);
    }

    [[nodiscard]] constexpr std::optional<EffectKind> ParseEffect(std::string_view name) noexcept
    {
        const auto it = std::ranges::find(effect_names, name);
        if (it == effect_names.end())
            return std::nullopt;
        return static_cast<EffectKind>(it - effect_names.begin());
    }

    [[nodiscard]] std::span<const EffectParameter> EffectParameters(EffectKind kind) noexcept;
    [[nodiscard]] std::unique_ptr<AudioEffect> CreateEffect(EffectKind kind);
} // namespace atmo::server::audio
