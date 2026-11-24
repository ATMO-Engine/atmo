#include "ducker.hpp"

#include <algorithm>
#include <cmath>

#include "server/audio/effects/dsp.hpp"

namespace atmo::server::audio
{
    Ducker::Ducker() : m_level_decay(static_cast<float>(1.0 - dsp::Smoothing(0.05))) {}

    void Ducker::setParameters(std::span<const float> values)
    {
        m_ducked = 1.0f - values[0];
        m_threshold = values[1];
        m_attack = dsp::Smoothing(values[2]);
        m_release = dsp::Smoothing(values[3]);
    }

    float Ducker::next(float left, float right) noexcept
    {
        // The level falls slowly so the zero crossings of a wave do not count as silence.
        m_level = dsp::Flush(std::max({ std::abs(left), std::abs(right), m_level * m_level_decay }));

        const double target = m_level > m_threshold ? m_ducked : 1.0;
        m_gain += (target - m_gain) * (target < m_gain ? m_attack : m_release);
        if (std::abs(target - m_gain) < 1e-4)
            m_gain = target;
        return static_cast<float>(m_gain);
    }
} // namespace atmo::server::audio
