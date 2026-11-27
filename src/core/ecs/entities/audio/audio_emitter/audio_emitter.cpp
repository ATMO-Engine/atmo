#include "audio_emitter.hpp"

#include <cmath>

#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "core/resource/resource_manager.hpp"
#include "meta/auto_register.hpp"
#include "server/audio/audio_server.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    namespace
    {
        bool Changed(float current, float sent)
        {
            return current != sent && !(std::isnan(current) && std::isnan(sent));
        }

        void EnsureVoice(components::AudioEmitter &emitter)
        {
            if (emitter.m_voice.isValid())
                return;

            const components::AudioEmitter defaults;
            emitter.m_voice.create();
            emitter.m_volume = defaults.m_volume;
            emitter.m_pitch = defaults.m_pitch;
            emitter.m_pan = defaults.m_pan;
            emitter.m_loop = defaults.m_loop;
            emitter.m_bus = defaults.m_bus;
            if (emitter.m_sample)
                server::audio::AudioServer::Instance().voiceSetSample(emitter.m_voice.rid(), emitter.m_sample->rid());
        }

        void LoadAudio(components::AudioEmitter &emitter)
        {
            auto &server = server::audio::AudioServer::Instance();
            EnsureVoice(emitter);

            server.voiceSetSample(emitter.m_voice.rid(), {});
            emitter.prev_audio_path = emitter.audio_path;
            emitter.m_sample = nullptr;

            if (emitter.audio_path.empty())
                return;

            try {
                emitter.m_sample = resource::ResourceManager::GetInstance().getResource<resource::AudioSample>(emitter.audio_path);
            } catch (const std::exception &e) {
                spdlog::error("Failed to open audio for emitter: {}", e.what());
                return;
            }

            server.voiceSetSample(emitter.m_voice.rid(), emitter.m_sample->rid());
        }

        server::audio::Bus EmitterBus(const components::AudioEmitter &emitter)
        {
            return server::audio::ParseBus(emitter.bus).value_or(server::audio::Bus::Master);
        }

        void Sync(components::AudioEmitter &emitter)
        {
            auto &server = server::audio::AudioServer::Instance();
            EnsureVoice(emitter);
            if (emitter.audio_path != emitter.prev_audio_path)
                LoadAudio(emitter);

            const server::audio::VoiceRid voice = emitter.m_voice.rid();
            if (Changed(emitter.volume, emitter.m_volume)) {
                emitter.m_volume = emitter.volume;
                server.voiceSetVolume(voice, emitter.volume);
            }
            if (Changed(emitter.pitch, emitter.m_pitch)) {
                emitter.m_pitch = emitter.pitch;
                server.voiceSetPitch(voice, emitter.pitch);
            }
            if (Changed(emitter.pan, emitter.m_pan)) {
                emitter.m_pan = emitter.pan;
                server.voiceSetPan(voice, emitter.pan);
            }
            if (emitter.loop != emitter.m_loop) {
                emitter.m_loop = emitter.loop;
                server.voiceSetLoop(voice, emitter.loop);
            }
            if (emitter.bus != emitter.m_bus) {
                emitter.m_bus = emitter.bus;
                if (!server::audio::ParseBus(emitter.bus))
                    spdlog::warn("Unknown audio bus '{}', expected master, music or sfx", emitter.bus);
                server.voiceSetBus(voice, EmitterBus(emitter));
            }
        }

        void Play(components::AudioEmitter &emitter)
        {
            Sync(emitter);
            emitter.m_autoplay_done = true;
            server::audio::AudioServer::Instance().voicePlay(emitter.m_voice.rid());
        }
    } // namespace

    void AudioEmitter::RegisterSystems(flecs::world *world)
    {
        world->system<components::AudioEmitter>("AudioEmitter_Sync").kind(flecs::PreUpdate).each([](flecs::entity e, components::AudioEmitter &emitter) {
            Sync(emitter);

            if (!emitter.autoplay || emitter.m_autoplay_done || !emitter.m_sample)
                return;

            const components::WorldContext *ctx = e.world().try_get<components::WorldContext>();
            if (!ctx || !ctx->is_editor_isolated)
                Play(emitter);
        });
    }

    void AudioEmitter::initialize()
    {
        Entity::initialize();

        setComponent<components::AudioEmitter>({});
    }

    void AudioEmitter::setAudioPath(const std::string &path)
    {
        auto emitter = p_handle.get_ref<components::AudioEmitter>();
        emitter->audio_path = path;
        Sync(*emitter.get());
    }

    std::string_view AudioEmitter::getAudioPath() const noexcept
    {
        return p_handle.get_ref<components::AudioEmitter>()->audio_path;
    }

    void AudioEmitter::play()
    {
        Play(*p_handle.get_ref<components::AudioEmitter>().get());
    }

    void AudioEmitter::stop()
    {
        auto emitter = p_handle.get_ref<components::AudioEmitter>();
        if (emitter->m_voice.isValid())
            server::audio::AudioServer::Instance().voiceStop(emitter->m_voice.rid());
    }

    void AudioEmitter::setPaused(bool paused)
    {
        auto emitter = p_handle.get_ref<components::AudioEmitter>();
        if (emitter->m_voice.isValid())
            server::audio::AudioServer::Instance().voiceSetPaused(emitter->m_voice.rid(), paused);
    }

    bool AudioEmitter::isPlaying() const
    {
        return server::audio::AudioServer::Instance().voiceIsPlaying(p_handle.get_ref<components::AudioEmitter>()->m_voice.rid());
    }

    void AudioEmitter::setVolume(float volume)
    {
        p_handle.get_ref<components::AudioEmitter>()->volume = volume;
    }

    float AudioEmitter::getVolume() const noexcept
    {
        return p_handle.get_ref<components::AudioEmitter>()->volume;
    }

    void AudioEmitter::setPitch(float pitch)
    {
        p_handle.get_ref<components::AudioEmitter>()->pitch = pitch;
    }

    float AudioEmitter::getPitch() const noexcept
    {
        return p_handle.get_ref<components::AudioEmitter>()->pitch;
    }

    void AudioEmitter::setPan(float pan)
    {
        p_handle.get_ref<components::AudioEmitter>()->pan = pan;
    }

    float AudioEmitter::getPan() const noexcept
    {
        return p_handle.get_ref<components::AudioEmitter>()->pan;
    }

    void AudioEmitter::setLoop(bool loop)
    {
        p_handle.get_ref<components::AudioEmitter>()->loop = loop;
    }

    bool AudioEmitter::getLoop() const noexcept
    {
        return p_handle.get_ref<components::AudioEmitter>()->loop;
    }

    void AudioEmitter::setAutoplay(bool autoplay)
    {
        p_handle.get_ref<components::AudioEmitter>()->autoplay = autoplay;
    }

    bool AudioEmitter::getAutoplay() const noexcept
    {
        return p_handle.get_ref<components::AudioEmitter>()->autoplay;
    }

    void AudioEmitter::setBus(server::audio::Bus bus)
    {
        p_handle.get_ref<components::AudioEmitter>()->bus = server::audio::BusName(bus);
    }

    server::audio::Bus AudioEmitter::getBus() const noexcept
    {
        return EmitterBus(*p_handle.get_ref<components::AudioEmitter>().get());
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::AudioEmitter);
ATMO_REGISTER_COMPONENT(atmo::core::components::AudioEmitter)
