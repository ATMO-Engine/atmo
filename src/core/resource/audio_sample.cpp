#include "audio_sample.hpp"

#include "server/audio/audio_server.hpp"

namespace atmo::core::resource
{
    AudioSample::AudioSample(server::audio::SampleRid rid) noexcept : m_rid(rid) {}

    AudioSample::~AudioSample()
    {
        if (m_rid.isValid())
            server::audio::AudioServer::Instance().sampleFree(m_rid);
    }

    server::audio::SampleRid AudioSample::rid() const noexcept
    {
        return m_rid;
    }

    double AudioSample::duration() const
    {
        const auto frames = server::audio::AudioServer::Instance().sampleGetFrames(m_rid);
        return static_cast<double>(frames) / server::audio::AudioServer::sample_rate;
    }
} // namespace atmo::core::resource
