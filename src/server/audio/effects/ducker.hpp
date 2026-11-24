#pragma once

#include <array>
#include <span>

#include "server/audio/effects/audio_effect.hpp"

namespace atmo::server::audio
{
    /**
     * @brief Gain of a bus that is turned down while another one is heard.
     */
    class Ducker
    {
    public:
        static constexpr std::array<EffectParameter, 4> parameters{ {
            { .name = "amount", .min = 0.0f, .max = 1.0f, .initial = 0.7f },
            { .name = "threshold", .min = 0.0f, .max = 1.0f, .initial = 0.02f },
            { .name = "attack", .min = 0.001f, .max = 5.0f, .initial = 0.02f },
            { .name = "release", .min = 0.001f, .max = 5.0f, .initial = 0.4f },
        } };

        Ducker();

        void setParameters(std::span<const float> values);

        /**
         * @brief Gain for the next frame.
         *
         * @param left Frame of the trigger bus
         */
        [[nodiscard]] float next(float left, float right) noexcept;

    private:
        float m_ducked = 1.0f;
        float m_threshold = 0.0f;
        // double: the last steps of a slow release are too small for a float, which would never reach 1.
        double m_attack = 1.0;
        double m_release = 1.0;
        float m_level_decay = 0.0f;
        float m_level = 0.0f;
        double m_gain = 1.0;
    };
} // namespace atmo::server::audio
