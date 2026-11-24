#include "audio_server.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "SDL3/SDL_init.h"
#include "server/audio/effects/dsp.hpp"
#include "spdlog/spdlog.h"

namespace atmo::server::audio
{
    namespace
    {
        constexpr int frame_bytes = static_cast<int>(sizeof(float)) * AudioServer::channels;

        bool CheckFinite(float value, const char *call)
        {
            if (std::isfinite(value))
                return true;
            spdlog::warn("{}: ignoring non-finite value {}", call, value);
            return false;
        }

        bool CheckBus(Bus bus, const char *call)
        {
            if (BusIndex(bus) < bus_names.size())
                return true;
            spdlog::warn("{}: unknown bus {}", call, BusIndex(bus));
            return false;
        }

        bool CheckEffect(EffectKind kind, const char *call)
        {
            if (EffectIndex(kind) < effect_names.size())
                return true;
            spdlog::warn("{}: unknown effect {}", call, EffectIndex(kind));
            return false;
        }

        void SetInitialValues(std::span<const EffectParameter> parameters, std::span<float> values)
        {
            for (std::size_t i = 0; i < parameters.size(); ++i) values[i] = parameters[i].initial;
        }

        void SetValues(std::span<const EffectParameter> parameters, std::span<const EffectValue> changes, std::span<float> values, const char *call)
        {
            for (const EffectValue &change : changes) {
                const auto it = std::ranges::find(parameters, change.name, &EffectParameter::name);
                if (it == parameters.end())
                    spdlog::warn("{}: unknown parameter '{}'", call, change.name);
                else if (CheckFinite(change.value, call))
                    values[static_cast<std::size_t>(it - parameters.begin())] = std::clamp(change.value, it->min, it->max);
            }
        }
    } // namespace

    AudioServer::AudioServer() = default;

    AudioServer::~AudioServer()
    {
        if (SDL_WasInit(SDL_INIT_AUDIO))
            shutdown();
    }

    AudioServer &AudioServer::Instance()
    {
        static AudioServer instance;
        return instance;
    }

    bool AudioServer::init()
    {
        if (m_initialized)
            return true;

        const SDL_AudioSpec spec{ .format = SDL_AUDIO_F32, .channels = channels, .freq = sample_rate };
        m_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, Callback, this);
        if (!m_stream) {
            spdlog::error("AudioServer: failed to open audio device: {}", SDL_GetError());
            return false;
        }
        auto resume = SDL_ResumeAudioStreamDevice(m_stream);
        if (!resume) {
            spdlog::error("AudioServer: failed to launch audio: {}", SDL_GetError());
            SDL_DestroyAudioStream(m_stream);
            m_stream = nullptr;
            return false;
        }
        m_initialized = true;

        const char *driver = SDL_GetCurrentAudioDriver();
        const char *device = SDL_GetAudioDeviceName(SDL_GetAudioStreamDevice(m_stream));
        spdlog::info(
            "AudioServer initialized with the {} driver ({}), {} Hz, {} channels",
            driver ? driver : "unknown",
            device ? device : "default device",
            sample_rate,
            channels);
        return true;
    }

    void AudioServer::shutdown()
    {
        if (!m_initialized)
            return;
        SDL_DestroyAudioStream(m_stream);
        m_stream = nullptr;
        m_initialized = false;
    }

    bool AudioServer::isInitialized() const noexcept
    {
        return m_initialized;
    }

    SampleRid AudioServer::sampleCreate(std::vector<float> pcm)
    {
        if (pcm.size() % channels != 0) {
            spdlog::error("sampleCreate: expected interleaved stereo data, got {} floats", pcm.size());
            return {};
        }

        std::lock_guard lock(m_mutex);
        const SampleRid sample = m_sample_rids.allocate();
        m_samples[sample.raw()] = Sample{ std::move(pcm) };
        return sample;
    }

    void AudioServer::sampleFree(SampleRid sample)
    {
        std::lock_guard lock(m_mutex);
        if (!m_sample_rids.release(sample)) {
            spdlog::warn("sampleFree: sample RID {:#x} is not alive", sample.raw());
            return;
        }
        m_samples.erase(sample.raw());

        for (auto &[id, voice] : m_voices) {
            if (voice.sample == sample) {
                voice.sample = {};
                voice.playing = false;
                voice.cursor = 0.0;
            }
        }
    }

    std::size_t AudioServer::sampleGetFrames(SampleRid sample) const
    {
        std::lock_guard lock(m_mutex);
        if (!m_sample_rids.isAlive(sample))
            return 0;
        return m_samples.at(sample.raw()).frames();
    }

    VoiceRid AudioServer::voiceCreate()
    {
        std::lock_guard lock(m_mutex);
        const VoiceRid voice = m_voice_rids.allocate();
        m_voices[voice.raw()] = Voice{};
        return voice;
    }

    void AudioServer::voiceFree(VoiceRid voice)
    {
        std::lock_guard lock(m_mutex);
        if (!m_voice_rids.release(voice)) {
            spdlog::warn("voiceFree: voice RID {:#x} is not alive", voice.raw());
            return;
        }
        m_voices.erase(voice.raw());
    }

    void AudioServer::voiceSetSample(VoiceRid voice, SampleRid sample)
    {
        std::lock_guard lock(m_mutex);
        Voice *v = findVoice(voice, "voiceSetSample");
        if (!v)
            return;
        if (sample.isValid() && !m_sample_rids.isAlive(sample)) {
            spdlog::warn("voiceSetSample: sample RID {:#x} is not alive", sample.raw());
            return;
        }
        v->sample = sample;
        v->cursor = 0.0;
        if (!sample.isValid())
            v->playing = false;
    }

    void AudioServer::voicePlay(VoiceRid voice)
    {
        std::lock_guard lock(m_mutex);
        if (Voice *v = findVoice(voice, "voicePlay")) {
            v->cursor = 0.0;
            v->playing = v->sample.isValid();
            v->paused = false;
        }
    }

    void AudioServer::voiceStop(VoiceRid voice)
    {
        std::lock_guard lock(m_mutex);
        if (Voice *v = findVoice(voice, "voiceStop")) {
            v->playing = false;
            v->paused = false;
            v->cursor = 0.0;
        }
    }

    void AudioServer::voiceSetPaused(VoiceRid voice, bool paused)
    {
        std::lock_guard lock(m_mutex);
        if (Voice *v = findVoice(voice, "voiceSetPaused"))
            v->paused = paused;
    }

    void AudioServer::voiceSetVolume(VoiceRid voice, float volume)
    {
        if (!CheckFinite(volume, "voiceSetVolume"))
            return;

        std::lock_guard lock(m_mutex);
        if (Voice *v = findVoice(voice, "voiceSetVolume"))
            v->volume = std::max(volume, 0.0f);
    }

    void AudioServer::voiceSetPitch(VoiceRid voice, float pitch)
    {
        if (!CheckFinite(pitch, "voiceSetPitch"))
            return;

        std::lock_guard lock(m_mutex);
        if (Voice *v = findVoice(voice, "voiceSetPitch"))
            v->pitch = std::max(pitch, 0.01f);
    }

    void AudioServer::voiceSetPan(VoiceRid voice, float pan)
    {
        if (!CheckFinite(pan, "voiceSetPan"))
            return;

        std::lock_guard lock(m_mutex);
        if (Voice *v = findVoice(voice, "voiceSetPan"))
            v->pan = std::clamp(pan, -1.0f, 1.0f);
    }

    void AudioServer::voiceSetLoop(VoiceRid voice, bool loop)
    {
        std::lock_guard lock(m_mutex);
        if (Voice *v = findVoice(voice, "voiceSetLoop"))
            v->loop = loop;
    }

    void AudioServer::voiceSetBus(VoiceRid voice, Bus bus)
    {
        if (!CheckBus(bus, "voiceSetBus"))
            return;

        std::lock_guard lock(m_mutex);
        if (Voice *v = findVoice(voice, "voiceSetBus"))
            v->bus = bus;
    }

    bool AudioServer::voiceIsPlaying(VoiceRid voice) const
    {
        std::lock_guard lock(m_mutex);
        if (!m_voice_rids.isAlive(voice))
            return false;
        return m_voices.at(voice.raw()).playing;
    }

    void AudioServer::busSetVolume(Bus bus, float volume)
    {
        if (!CheckBus(bus, "busSetVolume") || !CheckFinite(volume, "busSetVolume"))
            return;

        std::lock_guard lock(m_mutex);
        m_buses[BusIndex(bus)].volume = std::max(volume, 0.0f);
    }

    float AudioServer::busGetVolume(Bus bus) const
    {
        if (!CheckBus(bus, "busGetVolume"))
            return 0.0f;

        std::lock_guard lock(m_mutex);
        return m_buses[BusIndex(bus)].volume;
    }

    void AudioServer::busSetEffect(Bus bus, EffectKind kind, std::span<const EffectValue> values)
    {
        if (!CheckBus(bus, "busSetEffect") || !CheckEffect(kind, "busSetEffect"))
            return;

        std::unique_ptr<AudioEffect> created;
        if (!busGetEffect(bus, kind))
            created = CreateEffect(kind);

        const std::span<const EffectParameter> parameters = EffectParameters(kind);
        std::lock_guard lock(m_mutex);
        EffectSlot &slot = m_buses[BusIndex(bus)].effects[EffectIndex(kind)];
        if (!slot.effect) {
            slot.effect = created ? std::move(created) : CreateEffect(kind);
            SetInitialValues(parameters, slot.values);
        }
        SetValues(parameters, values, slot.values, "busSetEffect");
        slot.effect->setParameters(std::span(slot.values).first(parameters.size()));
    }

    void AudioServer::busRemoveEffect(Bus bus, EffectKind kind)
    {
        if (!CheckBus(bus, "busRemoveEffect") || !CheckEffect(kind, "busRemoveEffect"))
            return;

        std::unique_ptr<AudioEffect> removed;
        std::lock_guard lock(m_mutex);
        removed = std::move(m_buses[BusIndex(bus)].effects[EffectIndex(kind)].effect);
    }

    std::optional<std::vector<EffectValue>> AudioServer::busGetEffect(Bus bus, EffectKind kind) const
    {
        if (!CheckBus(bus, "busGetEffect") || !CheckEffect(kind, "busGetEffect"))
            return std::nullopt;

        std::lock_guard lock(m_mutex);
        const EffectSlot &slot = m_buses[BusIndex(bus)].effects[EffectIndex(kind)];
        if (!slot.effect)
            return std::nullopt;

        std::vector<EffectValue> values;
        for (const EffectParameter &parameter : EffectParameters(kind)) values.push_back({ parameter.name, slot.values[values.size()] });
        return values;
    }

    void AudioServer::busSetDucking(Bus bus, Bus trigger, std::span<const EffectValue> values)
    {
        if (!CheckBus(bus, "busSetDucking") || !CheckBus(trigger, "busSetDucking"))
            return;
        if (bus == trigger || bus == Bus::Master || trigger == Bus::Master) {
            spdlog::warn("busSetDucking: expected two different buses other than master, got {} and {}", BusName(bus), BusName(trigger));
            return;
        }

        std::lock_guard lock(m_mutex);
        std::optional<Ducking> &ducking = m_buses[BusIndex(bus)].ducking;
        if (!ducking) {
            ducking.emplace();
            SetInitialValues(Ducker::parameters, ducking->values);
        }
        ducking->trigger = trigger;
        SetValues(Ducker::parameters, values, ducking->values, "busSetDucking");
        ducking->ducker.setParameters(ducking->values);
    }

    void AudioServer::busRemoveDucking(Bus bus)
    {
        if (!CheckBus(bus, "busRemoveDucking"))
            return;

        std::lock_guard lock(m_mutex);
        m_buses[BusIndex(bus)].ducking.reset();
    }

    AudioServer::Voice *AudioServer::findVoice(VoiceRid voice, const char *call)
    {
        if (!m_voice_rids.isAlive(voice)) {
            spdlog::warn("{}: voice RID {:#x} is not alive", call, voice.raw());
            return nullptr;
        }
        return &m_voices.at(voice.raw());
    }

    void AudioServer::mix(float *out, int frames)
    {
        const std::size_t size = static_cast<std::size_t>(frames) * channels;

        std::lock_guard lock(m_mutex);
        for (BusState &bus : m_buses) bus.buffer.assign(size, 0.0f);

        for (auto &[id, voice] : m_voices) {
            if (!voice.playing || voice.paused)
                continue;

            auto it = m_samples.find(voice.sample.raw());
            if (it == m_samples.end() || it->second.frames() == 0) {
                voice.playing = false;
                continue;
            }
            const std::vector<float> &pcm = it->second.pcm;
            const std::size_t total = it->second.frames();
            const double length = static_cast<double>(total);

            float *bus = m_buses[BusIndex(voice.bus)].buffer.data();
            const float gain_left = voice.volume * std::min(1.0f, 1.0f - voice.pan);
            const float gain_right = voice.volume * std::min(1.0f, 1.0f + voice.pan);

            for (int i = 0; i < frames; ++i) {
                if (voice.cursor >= length) {
                    if (!voice.loop) {
                        voice.playing = false;
                        voice.cursor = 0.0;
                        break;
                    }
                    voice.cursor = std::fmod(voice.cursor, length);
                }

                const auto frame = static_cast<std::size_t>(voice.cursor);
                const std::size_t next = frame + 1 < total ? frame + 1 : (voice.loop ? 0 : frame);
                const float t = static_cast<float>(voice.cursor - static_cast<double>(frame));

                const float left = pcm[frame * 2] + (pcm[next * 2] - pcm[frame * 2]) * t;
                const float right = pcm[frame * 2 + 1] + (pcm[next * 2 + 1] - pcm[frame * 2 + 1]) * t;

                bus[i * 2] += left * gain_left;
                bus[i * 2 + 1] += right * gain_right;
                voice.cursor += voice.pitch;
            }
        }

        BusState &master = m_buses[BusIndex(Bus::Master)];
        for (BusState &bus : m_buses) {
            if (&bus != &master)
                ProcessBus(bus);
        }

        for (BusState &bus : m_buses) {
            if (&bus == &master)
                continue;
            const float *trigger = bus.ducking ? m_buses[BusIndex(bus.ducking->trigger)].buffer.data() : nullptr;
            for (std::size_t i = 0; i < size; i += channels) {
                const float gain = trigger ? bus.ducking->ducker.next(trigger[i], trigger[i + 1]) : 1.0f;
                master.buffer[i] += bus.buffer[i] * gain;
                master.buffer[i + 1] += bus.buffer[i + 1] * gain;
            }
        }

        ProcessBus(master);
        m_limiter.process(master.buffer.data(), static_cast<std::size_t>(frames));
        std::ranges::copy(master.buffer, out);
    }

    void AudioServer::update(float delta_time)
    {
        if (m_initialized || !CheckFinite(delta_time, "update") || delta_time <= 0.0f)
            return;

        const double frames = static_cast<double>(delta_time) * sample_rate;

        std::lock_guard lock(m_mutex);
        for (auto &[id, voice] : m_voices) {
            if (!voice.playing || voice.paused)
                continue;

            const auto it = m_samples.find(voice.sample.raw());
            if (it == m_samples.end() || it->second.frames() == 0) {
                voice.playing = false;
                continue;
            }

            const double length = static_cast<double>(it->second.frames());
            voice.cursor += frames * voice.pitch;
            if (voice.cursor < length)
                continue;

            voice.playing = voice.loop;
            voice.cursor = voice.loop ? std::fmod(voice.cursor, length) : 0.0;
        }
    }

    void AudioServer::ProcessBus(BusState &bus)
    {
        for (float &sample : bus.buffer) sample = dsp::Sanitize(sample);
        for (EffectSlot &slot : bus.effects) {
            if (slot.effect)
                slot.effect->process(bus.buffer.data(), bus.buffer.size() / channels);
        }
        for (float &sample : bus.buffer) sample = dsp::Sanitize(sample * bus.volume);
    }

    void SDLCALL AudioServer::Callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int)
    {
        auto *self = static_cast<AudioServer *>(userdata);

        const int frames = additional_amount / frame_bytes;
        if (frames <= 0)
            return;

        self->m_mix_buffer.resize(static_cast<std::size_t>(frames) * channels);
        self->mix(self->m_mix_buffer.data(), frames);
        SDL_PutAudioStreamData(stream, self->m_mix_buffer.data(), frames * frame_bytes);
    }
} // namespace atmo::server::audio
