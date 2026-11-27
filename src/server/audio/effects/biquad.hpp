#pragma once

#include <array>

#include "server/audio/audio_format.hpp"

namespace atmo::server::audio
{
    /**
     * @brief Second order filter, with the coefficients of the Audio EQ Cookbook (Robert Bristow-Johnson).
     */
    class Biquad
    {
    public:
        void setLowPass(float cutoff, float resonance);
        void setHighPass(float cutoff, float resonance);
        void setLowShelf(float frequency, float gain_db);
        void setHighShelf(float frequency, float gain_db);
        void setPeak(float frequency, float q, float gain_db);

        [[nodiscard]] float process(float input, int channel) noexcept;

    private:
        struct State {
            double z1 = 0.0;
            double z2 = 0.0;
        };

        void set(double b0, double b1, double b2, double a0, double a1, double a2) noexcept;

        double m_b0 = 1.0;
        double m_b1 = 0.0;
        double m_b2 = 0.0;
        double m_a1 = 0.0;
        double m_a2 = 0.0;
        std::array<State, channels> m_state;
    };
} // namespace atmo::server::audio
