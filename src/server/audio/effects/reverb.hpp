#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "server/audio/audio_format.hpp"
#include "server/audio/effects/audio_effect.hpp"

namespace atmo::server::audio
{
    /**
     * @brief Parallel comb filters then all-pass filters, with the tunings of Freeverb (public domain).
     */
    class Reverb : public AudioEffect
    {
    public:
        static constexpr std::array<EffectParameter, 3> parameters{ {
            { .name = "room_size", .min = 0.0f, .max = 1.0f, .initial = 0.5f },
            { .name = "damping", .min = 0.0f, .max = 1.0f, .initial = 0.5f },
            { .name = "mix", .min = 0.0f, .max = 1.0f, .initial = 0.3f },
        } };

        Reverb();

        void process(float *stereo, std::size_t frames) override;
        void setParameters(std::span<const float> values) override;

    private:
        struct Line {
            std::vector<float> buffer;
            std::size_t position = 0;
            float damped = 0.0f;
        };

        struct Channel {
            std::array<Line, 8> combs;
            std::array<Line, 4> all_passes;
        };

        [[nodiscard]] float processChannel(Channel &channel, float input) noexcept;

        std::array<Channel, channels> m_channels;
        float m_feedback = 0.0f;
        float m_damping = 0.0f;
        float m_mix = 0.0f;
    };
} // namespace atmo::server::audio
