#pragma once

#include <memory>
#include <string>
#include <string_view>
#include "core/ecs/entity_registry.hpp"
#include "core/resource/audio_sample.hpp"
#include "meta/meta.hpp"
#include "server/audio/bus.hpp"
#include "server/audio/scoped_voice.hpp"

namespace atmo::core::components
{
    struct AudioEmitter {
        std::string audio_path;
        float volume = 1.0f;
        float pitch = 1.0f;
        float pan = 0.0f;
        bool loop = false;
        bool autoplay = false;
        std::string bus{ server::audio::BusName(server::audio::Bus::Master) };

        std::string prev_audio_path;
        std::shared_ptr<resource::AudioSample> m_sample;
        server::audio::ScopedVoice m_voice;
        float m_volume = 1.0f;
        float m_pitch = 1.0f;
        float m_pan = 0.0f;
        bool m_loop = false;
        std::string m_bus = bus;
        bool m_autoplay_done = false;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::AudioEmitter> {
    static constexpr const char *name = "AudioEmitter";
    static constexpr const char *category = "Audio";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::AudioEmitter::audio_path>("audio_path").withWidget("file_path"),
        atmo::meta::field<&atmo::core::components::AudioEmitter::volume>("volume"), atmo::meta::field<&atmo::core::components::AudioEmitter::pitch>("pitch"),
        atmo::meta::field<&atmo::core::components::AudioEmitter::pan>("pan"), atmo::meta::field<&atmo::core::components::AudioEmitter::loop>("loop"),
        atmo::meta::field<&atmo::core::components::AudioEmitter::autoplay>("autoplay"),
        atmo::meta::field<&atmo::core::components::AudioEmitter::bus>("bus").withWidget("audio_bus"));
};

namespace atmo::core::ecs::entities
{
    class AudioEmitter : public EntityRegistry::Registrable<AudioEmitter, Entity>
    {
    public:
        using EntityRegistry::Registrable<AudioEmitter, Entity>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "AudioEmitter";
        }

        void setAudioPath(const std::string &path);
        std::string_view getAudioPath() const noexcept;

        /**
         * @brief Play from the beginning, even if the emitter is already playing.
         */
        void play();
        void stop();
        void setPaused(bool paused);
        bool isPlaying() const;

        void setVolume(float volume);
        float getVolume() const noexcept;

        void setPitch(float pitch);
        float getPitch() const noexcept;

        void setPan(float pan);
        float getPan() const noexcept;

        void setLoop(bool loop);
        bool getLoop() const noexcept;

        void setAutoplay(bool autoplay);
        bool getAutoplay() const noexcept;

        void setBus(server::audio::Bus bus);
        server::audio::Bus getBus() const noexcept;
    };
} // namespace atmo::core::ecs::entities
