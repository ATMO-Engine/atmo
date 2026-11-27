#pragma once

#include <array>

#include "server/audio/effects/audio_effect.hpp"

namespace atmo::server::audio
{
    /**
     * @brief Turns the sound down as soon as it goes over the ceiling, then back up slowly. Adds no latency.
     */
    class Limiter : public AudioEffect
    {
    public:
        static constexpr std::array<EffectParameter, 2> parameters{ {
            { .name = "ceiling", .min = 0.01f, .max = 1.0f, .initial = 1.0f },
            { .name = "release", .min = 0.001f, .max = 5.0f, .initial = 0.1f },
        } };

        Limiter();

        void process(float *stereo, std::size_t frames) override;
        void setParameters(std::span<const float> values) override;

    private:
        float m_ceiling = 1.0f;
        double m_release = 1.0;
        double m_gain = 1.0;
    };
} // namespace atmo::server::audio
