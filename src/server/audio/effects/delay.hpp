#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "server/audio/effects/audio_effect.hpp"

namespace atmo::server::audio
{
    class Delay : public AudioEffect
    {
    public:
        static constexpr float max_time = 2.0f;
        static constexpr std::array<EffectParameter, 3> parameters{ {
            { .name = "time", .min = 0.001f, .max = max_time, .initial = 0.3f },
            { .name = "feedback", .min = 0.0f, .max = 0.95f, .initial = 0.4f },
            { .name = "mix", .min = 0.0f, .max = 1.0f, .initial = 0.3f },
        } };

        Delay();

        void process(float *stereo, std::size_t frames) override;
        void setParameters(std::span<const float> values) override;

    private:
        std::vector<float> m_line;
        std::size_t m_write = 0;
        std::size_t m_delay = 1;
        float m_feedback = 0.0f;
        float m_mix = 0.0f;
    };
} // namespace atmo::server::audio
