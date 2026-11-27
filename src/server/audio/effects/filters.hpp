#pragma once

#include <array>

#include "server/audio/effects/audio_effect.hpp"
#include "server/audio/effects/biquad.hpp"

namespace atmo::server::audio
{
    class LowPass : public AudioEffect
    {
    public:
        static constexpr std::array<EffectParameter, 2> parameters{ {
            { .name = "cutoff", .min = 20.0f, .max = 20000.0f, .initial = 1000.0f },
            { .name = "resonance", .min = 0.1f, .max = 10.0f, .initial = 0.707f },
        } };

        void process(float *stereo, std::size_t frames) override;
        void setParameters(std::span<const float> values) override;

    private:
        Biquad m_filter;
    };

    class HighPass : public AudioEffect
    {
    public:
        static constexpr std::array<EffectParameter, 2> parameters{ {
            { .name = "cutoff", .min = 20.0f, .max = 20000.0f, .initial = 500.0f },
            { .name = "resonance", .min = 0.1f, .max = 10.0f, .initial = 0.707f },
        } };

        void process(float *stereo, std::size_t frames) override;
        void setParameters(std::span<const float> values) override;

    private:
        Biquad m_filter;
    };

    /**
     * @brief Gain of the bass, the mids and the treble, in decibels.
     */
    class Eq3Band : public AudioEffect
    {
    public:
        static constexpr std::array<EffectParameter, 3> parameters{ {
            { .name = "low", .min = -24.0f, .max = 24.0f, .initial = 0.0f },
            { .name = "mid", .min = -24.0f, .max = 24.0f, .initial = 0.0f },
            { .name = "high", .min = -24.0f, .max = 24.0f, .initial = 0.0f },
        } };

        static constexpr float low_frequency = 250.0f;
        static constexpr float mid_frequency = 1000.0f;
        static constexpr float high_frequency = 4000.0f;

        void process(float *stereo, std::size_t frames) override;
        void setParameters(std::span<const float> values) override;

    private:
        Biquad m_low;
        Biquad m_mid;
        Biquad m_high;
    };
} // namespace atmo::server::audio
