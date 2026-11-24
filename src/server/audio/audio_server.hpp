#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "SDL3/SDL_audio.h"
#include "server/audio/audio_format.hpp"
#include "server/audio/bus.hpp"
#include "server/audio/effects/ducker.hpp"
#include "server/audio/effects/effect_kind.hpp"
#include "server/audio/effects/limiter.hpp"
#include "server/audio/rid.hpp"
#include "server/audio/rid_allocator.hpp"

namespace atmo::server::audio
{
    class AudioServer
    {
    public:
        static constexpr int sample_rate = audio::sample_rate;
        static constexpr int channels = audio::channels;

        static AudioServer &Instance();
        ~AudioServer();

        AudioServer(const AudioServer &) = delete;
        AudioServer &operator=(const AudioServer &) = delete;

        bool init();
        void shutdown();
        [[nodiscard]] bool isInitialized() const noexcept;

        /**
         * @brief Store decoded audio in the server.
         *
         * @param pcm Interleaved stereo float samples (L R L R ...) at sample_rate
         * @return SampleRid An invalid RID if the data is malformed
         */
        SampleRid sampleCreate(std::vector<float> pcm);
        void sampleFree(SampleRid sample);
        [[nodiscard]] std::size_t sampleGetFrames(SampleRid sample) const;

        VoiceRid voiceCreate();
        void voiceFree(VoiceRid voice);
        void voiceSetSample(VoiceRid voice, SampleRid sample);
        void voicePlay(VoiceRid voice);
        void voiceStop(VoiceRid voice);
        void voiceSetPaused(VoiceRid voice, bool paused);
        void voiceSetVolume(VoiceRid voice, float volume);
        void voiceSetPitch(VoiceRid voice, float pitch);
        void voiceSetPan(VoiceRid voice, float pan);
        void voiceSetLoop(VoiceRid voice, bool loop);
        void voiceSetBus(VoiceRid voice, Bus bus);
        [[nodiscard]] bool voiceIsPlaying(VoiceRid voice) const;

        /**
         * @brief Set the volume of every voice of a bus. Master applies on top of the other buses.
         */
        void busSetVolume(Bus bus, float volume);
        [[nodiscard]] float busGetVolume(Bus bus) const;

        /**
         * @brief Add an effect to a bus, or change its settings if the bus already has it.
         *
         * @param values Settings to change, the others keep their value
         */
        void busSetEffect(Bus bus, EffectKind kind, std::span<const EffectValue> values = {});
        void busRemoveEffect(Bus bus, EffectKind kind);

        /**
         * @brief Settings of an effect of a bus, nothing if the bus does not have it.
         */
        [[nodiscard]] std::optional<std::vector<EffectValue>> busGetEffect(Bus bus, EffectKind kind) const;

        /**
         * @brief Turn a bus down while another one is heard. Neither can be Master.
         *
         * @param values Settings to change, the others keep their value
         */
        void busSetDucking(Bus bus, Bus trigger, std::span<const EffectValue> values = {});
        void busRemoveDucking(Bus bus);

        /**
         * @brief Fill out with the next frames of every playing voice.
         * Called by the SDL callback on the audio thread; public so tests can drive it without a device.
         *
         * @param out Interleaved stereo buffer of frames * channels floats
         * @param frames Number of frames to produce
         */
        void mix(float *out, int frames);

        /**
         * @brief Move the playing voices forward when no audio device does it, so that sounds still end.
         *
         * @param delta_time Time since the last update, in seconds
         */
        void update(float delta_time);

    private:
        AudioServer();

        struct Sample {
            std::vector<float> pcm;

            [[nodiscard]] std::size_t frames() const noexcept
            {
                return pcm.size() / channels;
            }
        };

        struct Voice {
            SampleRid sample;
            double cursor = 0.0;
            float volume = 1.0f;
            float pitch = 1.0f;
            float pan = 0.0f;
            Bus bus = Bus::Master;
            bool loop = false;
            bool playing = false;
            bool paused = false;
        };

        struct EffectSlot {
            std::unique_ptr<AudioEffect> effect;
            std::array<float, max_effect_parameters> values{};
        };

        struct Ducking {
            Bus trigger = Bus::Master;
            Ducker ducker;
            std::array<float, max_effect_parameters> values{};
        };

        struct BusState {
            float volume = 1.0f;
            std::vector<float> buffer;
            std::array<EffectSlot, effect_names.size()> effects;
            std::optional<Ducking> ducking;
        };

        static void SDLCALL Callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);

        static void ProcessBus(BusState &bus);

        Voice *findVoice(VoiceRid voice, const char *call);

        bool m_initialized = false;
        SDL_AudioStream *m_stream = nullptr;
        std::vector<float> m_mix_buffer;

        mutable std::mutex m_mutex;
        RidAllocator<RidKind::Sample> m_sample_rids;
        RidAllocator<RidKind::Voice> m_voice_rids;
        std::unordered_map<std::uint32_t, Sample> m_samples;
        std::unordered_map<std::uint32_t, Voice> m_voices;
        std::array<BusState, bus_names.size()> m_buses;
        Limiter m_limiter;
    };
} // namespace atmo::server::audio
