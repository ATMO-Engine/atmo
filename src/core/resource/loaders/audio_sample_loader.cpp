#include "audio_sample_loader.hpp"

#include <cstddef>
#include <cstring>
#include <utility>
#include <vector>

#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_stdinc.h"
#include "core/resource/auto_register_loader.hpp"
#include "project/file_system.hpp"
#include "server/audio/audio_server.hpp"

namespace atmo::core::resource
{
    namespace
    {
        struct SdlFree {
            void operator()(Uint8 *data) const noexcept
            {
                SDL_free(data);
            }
        };
        using SdlBuffer = std::unique_ptr<Uint8, SdlFree>;
    } // namespace

    std::shared_ptr<AudioSample> AudioSampleLoader::load(const std::string &path)
    {
        const std::string bytes = project::FileSystem::OpenFile(path).readAll();

        SDL_AudioSpec source{};
        Uint8 *decoded_data = nullptr;
        Uint32 decoded_size = 0;
        if (!SDL_LoadWAV_IO(SDL_IOFromConstMem(bytes.data(), bytes.size()), true, &source, &decoded_data, &decoded_size))
            throw LoadException("Failed to decode audio " + path + ": " + SDL_GetError());
        const SdlBuffer decoded(decoded_data);

        const SDL_AudioSpec target{ .format = SDL_AUDIO_F32,
                                    .channels = server::audio::AudioServer::channels,
                                    .freq = server::audio::AudioServer::sample_rate };
        Uint8 *converted_data = nullptr;
        int converted_size = 0;
        if (!SDL_ConvertAudioSamples(&source, decoded.get(), static_cast<int>(decoded_size), &target, &converted_data, &converted_size))
            throw LoadException("Failed to convert audio " + path + ": " + SDL_GetError());
        const SdlBuffer converted(converted_data);

        std::vector<float> pcm(static_cast<std::size_t>(converted_size) / sizeof(float));
        if (pcm.empty())
            throw LoadException("Audio " + path + " is empty");
        std::memcpy(pcm.data(), converted.get(), pcm.size() * sizeof(float));

        const server::audio::SampleRid rid = server::audio::AudioServer::Instance().sampleCreate(std::move(pcm));
        if (!rid.isValid())
            throw LoadException("Failed to create the audio sample for " + path);

        return std::make_shared<AudioSample>(rid);
    }
} // namespace atmo::core::resource

ATMO_REGISTER_RESOURCE_LOADER(atmo::core::resource::AudioSample, atmo::core::resource::AudioSampleLoader, .display_name = "Audio", .extensions = { ".wav" });
