#include "filters.hpp"

namespace atmo::server::audio
{
    namespace
    {
        void ProcessFilter(Biquad &filter, float *stereo, std::size_t frames)
        {
            for (std::size_t i = 0; i < frames; ++i) {
                stereo[i * 2] = filter.process(stereo[i * 2], 0);
                stereo[i * 2 + 1] = filter.process(stereo[i * 2 + 1], 1);
            }
        }
    } // namespace

    void LowPass::process(float *stereo, std::size_t frames)
    {
        ProcessFilter(m_filter, stereo, frames);
    }

    void LowPass::setParameters(std::span<const float> values)
    {
        m_filter.setLowPass(values[0], values[1]);
    }

    void HighPass::process(float *stereo, std::size_t frames)
    {
        ProcessFilter(m_filter, stereo, frames);
    }

    void HighPass::setParameters(std::span<const float> values)
    {
        m_filter.setHighPass(values[0], values[1]);
    }

    void Eq3Band::process(float *stereo, std::size_t frames)
    {
        ProcessFilter(m_low, stereo, frames);
        ProcessFilter(m_mid, stereo, frames);
        ProcessFilter(m_high, stereo, frames);
    }

    void Eq3Band::setParameters(std::span<const float> values)
    {
        m_low.setLowShelf(low_frequency, values[0]);
        m_mid.setPeak(mid_frequency, 0.7f, values[1]);
        m_high.setHighShelf(high_frequency, values[2]);
    }
} // namespace atmo::server::audio
