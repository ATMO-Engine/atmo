#pragma once

#include "server/audio/rid.hpp"

namespace atmo::core::resource
{
    class AudioSample
    {
    public:
        explicit AudioSample(server::audio::SampleRid rid) noexcept;
        ~AudioSample();

        AudioSample(const AudioSample &) = delete;
        AudioSample &operator=(const AudioSample &) = delete;

        [[nodiscard]] server::audio::SampleRid rid() const noexcept;

        /**
         * @brief Length of the sound at normal pitch, in seconds.
         */
        [[nodiscard]] double duration() const;

    private:
        server::audio::SampleRid m_rid;
    };
} // namespace atmo::core::resource
