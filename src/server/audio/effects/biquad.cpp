#include "biquad.hpp"

#include <cmath>
#include <numbers>

#include "server/audio/effects/dsp.hpp"

namespace atmo::server::audio
{
    namespace
    {
        struct Angle {
            double cos;
            double alpha;

            Angle(float frequency, double q)
            {
                const double w = 2.0 * std::numbers::pi * frequency / sample_rate;
                cos = std::cos(w);
                alpha = std::sin(w) / (2.0 * q);
            }
        };

        double Amplitude(float gain_db)
        {
            return std::pow(10.0, gain_db / 40.0);
        }

        constexpr double shelf_q = std::numbers::sqrt2 / 2.0;
    } // namespace

    void Biquad::setLowPass(float cutoff, float resonance)
    {
        const Angle w(cutoff, resonance);
        const double b = 1.0 - w.cos;
        set(b / 2.0, b, b / 2.0, 1.0 + w.alpha, -2.0 * w.cos, 1.0 - w.alpha);
    }

    void Biquad::setHighPass(float cutoff, float resonance)
    {
        const Angle w(cutoff, resonance);
        const double b = 1.0 + w.cos;
        set(b / 2.0, -b, b / 2.0, 1.0 + w.alpha, -2.0 * w.cos, 1.0 - w.alpha);
    }

    void Biquad::setLowShelf(float frequency, float gain_db)
    {
        const Angle w(frequency, shelf_q);
        const double a = Amplitude(gain_db);
        const double s = 2.0 * std::sqrt(a) * w.alpha;
        set(a * ((a + 1.0) - (a - 1.0) * w.cos + s),
            2.0 * a * ((a - 1.0) - (a + 1.0) * w.cos),
            a * ((a + 1.0) - (a - 1.0) * w.cos - s),
            (a + 1.0) + (a - 1.0) * w.cos + s,
            -2.0 * ((a - 1.0) + (a + 1.0) * w.cos),
            (a + 1.0) + (a - 1.0) * w.cos - s);
    }

    void Biquad::setHighShelf(float frequency, float gain_db)
    {
        const Angle w(frequency, shelf_q);
        const double a = Amplitude(gain_db);
        const double s = 2.0 * std::sqrt(a) * w.alpha;
        set(a * ((a + 1.0) + (a - 1.0) * w.cos + s),
            -2.0 * a * ((a - 1.0) + (a + 1.0) * w.cos),
            a * ((a + 1.0) + (a - 1.0) * w.cos - s),
            (a + 1.0) - (a - 1.0) * w.cos + s,
            2.0 * ((a - 1.0) - (a + 1.0) * w.cos),
            (a + 1.0) - (a - 1.0) * w.cos - s);
    }

    void Biquad::setPeak(float frequency, float q, float gain_db)
    {
        const Angle w(frequency, q);
        const double a = Amplitude(gain_db);
        set(1.0 + w.alpha * a, -2.0 * w.cos, 1.0 - w.alpha * a, 1.0 + w.alpha / a, -2.0 * w.cos, 1.0 - w.alpha / a);
    }

    float Biquad::process(float input, int channel) noexcept
    {
        State &s = m_state[channel];
        const double output = m_b0 * input + s.z1;
        s.z1 = m_b1 * input - m_a1 * output + s.z2;
        s.z2 = m_b2 * input - m_a2 * output;
        if (dsp::Flush(s.z1) == 0.0 && dsp::Flush(s.z2) == 0.0)
            s = {};
        return static_cast<float>(output);
    }

    void Biquad::set(double b0, double b1, double b2, double a0, double a1, double a2) noexcept
    {
        m_b0 = b0 / a0;
        m_b1 = b1 / a0;
        m_b2 = b2 / a0;
        m_a1 = a1 / a0;
        m_a2 = a2 / a0;
    }
} // namespace atmo::server::audio
