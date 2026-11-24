#pragma once

#include <algorithm>
#include <cmath>

#include "server/audio/audio_format.hpp"

namespace atmo::server::audio::dsp
{
    [[nodiscard]] inline float Sanitize(float value) noexcept
    {
        constexpr float limit = 1e6f;
        return std::isnan(value) ? 0.0f : std::clamp(value, -limit, limit);
    }

    template <typename T> [[nodiscard]] T Flush(T value) noexcept
    {
        return std::abs(value) < T(1e-15) ? T(0) : value;
    }

    /**
     * @brief Coefficient of a one-pole smoother that covers 63% of the distance in the given time.
     */
    [[nodiscard]] inline double Smoothing(double seconds) noexcept
    {
        return 1.0 - std::exp(-1.0 / (seconds * sample_rate));
    }
} // namespace atmo::server::audio::dsp
