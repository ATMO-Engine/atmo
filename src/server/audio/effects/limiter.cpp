#include "limiter.hpp"

#include <algorithm>
#include <cmath>

#include "server/audio/effects/dsp.hpp"

namespace atmo::server::audio
{
    Limiter::Limiter()
    {
        setParameters(std::array{ parameters[0].initial, parameters[1].initial });
    }

    void Limiter::process(float *stereo, std::size_t frames)
    {
        for (std::size_t i = 0; i < frames; ++i) {
            float &left = stereo[i * 2];
            float &right = stereo[i * 2 + 1];
            const float peak = std::max(std::abs(left), std::abs(right));

            m_gain += (1.0 - m_gain) * m_release;
            if (m_gain > 0.9999)
                m_gain = 1.0;
            if (peak * m_gain > m_ceiling)
                m_gain = static_cast<double>(m_ceiling) / peak;
            const auto gain = static_cast<float>(m_gain);

            left = std::clamp(left * gain, -m_ceiling, m_ceiling);
            right = std::clamp(right * gain, -m_ceiling, m_ceiling);
        }
    }

    void Limiter::setParameters(std::span<const float> values)
    {
        m_ceiling = values[0];
        m_release = dsp::Smoothing(values[1]);
    }
} // namespace atmo::server::audio
