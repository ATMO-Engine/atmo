#include "reverb.hpp"

#include "server/audio/effects/dsp.hpp"

namespace atmo::server::audio
{
    namespace
    {
        // Lengths of the filters in frames, for a sample rate of 44.1 kHz
        constexpr std::array<int, 8> comb_tunings{ 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        constexpr std::array<int, 4> all_pass_tunings{ 556, 441, 341, 225 };
        constexpr int stereo_spread = 23;

        constexpr float input_gain = 0.015f;
        constexpr float output_gain = 3.0f;

        std::size_t Frames(int tuning)
        {
            return static_cast<std::size_t>(tuning) * sample_rate / 44100;
        }
    } // namespace

    Reverb::Reverb()
    {
        for (std::size_t c = 0; c < channels; ++c) {
            const int spread = static_cast<int>(c) * stereo_spread;
            for (std::size_t i = 0; i < comb_tunings.size(); ++i) m_channels[c].combs[i].buffer.resize(Frames(comb_tunings[i] + spread));
            for (std::size_t i = 0; i < all_pass_tunings.size(); ++i) m_channels[c].all_passes[i].buffer.resize(Frames(all_pass_tunings[i] + spread));
        }
    }

    float Reverb::processChannel(Channel &channel, float input) noexcept
    {
        float output = 0.0f;
        for (Line &comb : channel.combs) {
            float &slot = comb.buffer[comb.position];
            const float delayed = slot;
            comb.damped = dsp::Flush(delayed + (comb.damped - delayed) * m_damping);
            slot = dsp::Flush(input + comb.damped * m_feedback);
            comb.position = (comb.position + 1) % comb.buffer.size();
            output += delayed;
        }

        for (Line &all_pass : channel.all_passes) {
            float &slot = all_pass.buffer[all_pass.position];
            const float delayed = slot;
            slot = dsp::Flush(output + delayed * 0.5f);
            all_pass.position = (all_pass.position + 1) % all_pass.buffer.size();
            output = delayed - output;
        }
        return output;
    }

    void Reverb::process(float *stereo, std::size_t frames)
    {
        for (std::size_t i = 0; i < frames; ++i) {
            const float input = (stereo[i * 2] + stereo[i * 2 + 1]) * input_gain;
            for (std::size_t c = 0; c < channels; ++c) {
                float &sample = stereo[i * channels + c];
                sample += (processChannel(m_channels[c], input) * output_gain - sample) * m_mix;
            }
        }
    }

    void Reverb::setParameters(std::span<const float> values)
    {
        m_feedback = 0.7f + values[0] * 0.28f;
        m_damping = values[1] * 0.4f;
        m_mix = values[2];
    }
} // namespace atmo::server::audio
