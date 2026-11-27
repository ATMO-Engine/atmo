#pragma once

#include <memory>
#include <string>

#include "core/resource/audio_sample.hpp"
#include "core/resource/resource.hpp"

namespace atmo::core::resource
{
    class AudioSampleLoader : public Resource<AudioSample>
    {
    public:
        std::shared_ptr<AudioSample> load(const std::string &path) override;
    };
} // namespace atmo::core::resource
