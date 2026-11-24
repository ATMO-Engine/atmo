#include "delay.hpp"

#include <algorithm>
#include <cmath>

#include "server/audio/effects/dsp.hpp"

namespace atmo::server::audio
{
    namespace
    {
        constexpr std::size_t line_frames = static_cast<std::size_t>(Delay::max_time * sample_rate) + 1;
    } // namespace

    Delay::Delay() : m_line(line_frames * channels, 0.0f) {}

    void Delay::process(float *stereo, std::size_t frames)
    {
        for (std::size_t i = 0; i < frames; ++i) {
            const std::size_t read = (m_write + line_frames - m_delay) % line_frames;
            for (std::size_t c = 0; c < channels; ++c) {
                float &sample = stereo[i * channels + c];
                const float echo = m_line[read * channels + c];
                m_line[m_write * channels + c] = dsp::Flush(sample + echo * m_feedback);
                sample += (echo - sample) * m_mix;
            }
            m_write = (m_write + 1) % line_frames;
        }
    }

    void Delay::setParameters(std::span<const float> values)
    {
        m_delay = std::clamp<std::size_t>(std::lround(values[0] * static_cast<float>(sample_rate)), 1, line_frames - 1);
        m_feedback = values[1];
        m_mix = values[2];
    }
} // namespace atmo::server::audio
