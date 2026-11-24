#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/ecs/components.hpp"
#include "core/ecs/entities/audio/audio_emitter/audio_emitter.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "meta/meta_registry.hpp"
#include "server/audio/audio_server.hpp"
#include "server/audio/scoped_voice.hpp"

using atmo::core::components::AudioEmitter;
using atmo::core::components::WorldContext;
using atmo::core::ecs::EntityRegistry;
using atmo::meta::MetaRegistry;
using atmo::server::audio::AudioServer;
using atmo::server::audio::Bus;
using atmo::server::audio::ParseBus;
using atmo::server::audio::ScopedVoice;
using Emitter = atmo::core::ecs::entities::AudioEmitter;

namespace
{
    struct TestWorld {
        std::filesystem::path root = std::filesystem::temp_directory_path() / "atmo-audio-emitter-test";
        flecs::world world;

        static void Setup(flecs::world &world, bool editor)
        {
            atmo::core::components::register_core_components(world);
            atmo::core::ecs::entities::Entity::RegisterSystems(&world);
            world.set<WorldContext>({ .is_editor_isolated = editor });
            Emitter::RegisterSystems(&world);
        }

        struct Editor {
            flecs::world world;

            Editor()
            {
                Setup(world, true);
            }
        };

        explicit TestWorld(bool editor = false)
        {
            std::filesystem::remove_all(root);
            std::filesystem::create_directories(root);
            Setup(world, editor);
        }

        ~TestWorld()
        {
            std::error_code error;
            std::filesystem::remove_all(root, error);
        }

        std::shared_ptr<Emitter> create()
        {
            auto emitter = EntityRegistry::CreateIn<Emitter>(&world, "Entity::AudioEmitter");
            REQUIRE(emitter != nullptr);
            return emitter;
        }

        std::string wav(const std::string &name, float value, std::uint32_t frames = 4800) const
        {
            const auto append = [](std::string &out, std::uint32_t number, int bytes) {
                for (int i = 0; i < bytes; ++i) out.push_back(static_cast<char>((number >> (8 * i)) & 0xFF));
            };
            std::string out = "RIFF";
            append(out, 36 + frames * 4, 4);
            out += "WAVEfmt ";
            append(out, 16, 4);
            append(out, 1, 2);
            append(out, 2, 2);
            append(out, 48000, 4);
            append(out, 48000 * 4, 4);
            append(out, 4, 2);
            append(out, 16, 2);
            out += "data";
            append(out, frames * 4, 4);
            for (std::uint32_t i = 0; i < frames * 2; ++i) append(out, static_cast<std::uint16_t>(static_cast<std::int16_t>(value * 32768.0f)), 2);

            std::ofstream file(root / name, std::ios::binary | std::ios::trunc);
            file << out;
            return (root / name).generic_string();
        }
    };

    std::pair<float, float> MixFrame()
    {
        float out[2] = {};
        AudioServer::Instance().mix(out, 1);
        return { out[0], out[1] };
    }

    struct ScopedBusVolume {
        Bus bus;

        ScopedBusVolume(Bus bus, float volume) : bus(bus)
        {
            AudioServer::Instance().busSetVolume(bus, volume);
        }
        ~ScopedBusVolume()
        {
            AudioServer::Instance().busSetVolume(bus, 1.0f);
        }

        ScopedBusVolume(const ScopedBusVolume &) = delete;
        ScopedBusVolume &operator=(const ScopedBusVolume &) = delete;
    };

    bool Near(float a, float b)
    {
        return std::abs(a - b) < 1e-3f;
    }

    bool IsAlive(atmo::server::audio::VoiceRid voice)
    {
        auto &server = AudioServer::Instance();
        const auto sample = server.sampleCreate({ 0.0f, 0.0f });
        server.voiceSetSample(voice, sample);
        server.voicePlay(voice);
        const bool alive = server.voiceIsPlaying(voice);
        server.sampleFree(sample);
        return alive;
    }
} // namespace

TEST_CASE("AudioEmitter exposes its settings and nothing else", "[audio][ecs]")
{
    const auto *info = MetaRegistry::Instance().find("AudioEmitter");
    REQUIRE(info != nullptr);

    std::vector<std::string> names;
    for (const auto &field : info->fields) names.emplace_back(field.name);
    REQUIRE(names == std::vector<std::string>{ "audio_path", "volume", "pitch", "pan", "loop", "autoplay", "bus" });
    REQUIRE(std::string_view(info->fields[0].widget) == "file_path");
    REQUIRE(std::string_view(info->fields[6].widget) == "audio_bus");
}

TEST_CASE("AudioEmitter settings survive a save and runtime state does not", "[audio][ecs]")
{
    const auto *info = MetaRegistry::Instance().find("AudioEmitter");
    REQUIRE(info != nullptr);

    AudioEmitter saved;
    saved.audio_path = "project://sounds/shot.wav";
    saved.volume = 0.5f;
    saved.pitch = 1.5f;
    saved.pan = -0.25f;
    saved.loop = true;
    saved.autoplay = true;
    saved.bus = "music";
    saved.prev_audio_path = saved.audio_path;
    saved.m_voice.create();
    saved.m_autoplay_done = true;
    const std::string json = info->to_json(&saved).dump().value();

    AudioEmitter loaded;
    REQUIRE(info->from_json(&loaded, json));
    REQUIRE(loaded.audio_path == saved.audio_path);
    REQUIRE(loaded.volume == saved.volume);
    REQUIRE(loaded.pitch == saved.pitch);
    REQUIRE(loaded.pan == saved.pan);
    REQUIRE(loaded.loop);
    REQUIRE(loaded.autoplay);
    REQUIRE(loaded.bus == "music");

    REQUIRE(loaded.prev_audio_path.empty());
    REQUIRE(loaded.m_sample == nullptr);
    REQUIRE_FALSE(loaded.m_voice.isValid());
    REQUIRE_FALSE(loaded.m_autoplay_done);
}

TEST_CASE("loading AudioEmitter settings keeps the live voice", "[audio][ecs]")
{
    const auto *info = MetaRegistry::Instance().find("AudioEmitter");
    REQUIRE(info != nullptr);

    AudioEmitter emitter;
    emitter.m_voice.create();
    const auto voice = emitter.m_voice.rid();

    REQUIRE(info->from_json(&emitter, R"({"volume":0.25})"));
    REQUIRE(emitter.volume == 0.25f);
    REQUIRE(emitter.m_voice.rid() == voice);
}

TEST_CASE("a new AudioEmitter matches the defaults of a new voice", "[audio][ecs]")
{
    auto &server = AudioServer::Instance();
    const AudioEmitter emitter;
    const auto sample = server.sampleCreate({ 0.5f, -0.5f, 0.25f, 0.0f });
    const auto voice = server.voiceCreate();
    server.voiceSetSample(voice, sample);
    server.voicePlay(voice);

    std::vector<float> untouched(6);
    server.mix(untouched.data(), 3);

    server.voiceSetVolume(voice, emitter.m_volume);
    server.voiceSetPitch(voice, emitter.m_pitch);
    server.voiceSetPan(voice, emitter.m_pan);
    server.voiceSetLoop(voice, emitter.m_loop);
    server.voiceSetBus(voice, ParseBus(emitter.m_bus).value());
    server.voicePlay(voice);
    std::vector<float> sent(6);
    server.mix(sent.data(), 3);

    REQUIRE(sent == untouched);
    server.voiceFree(voice);
    server.sampleFree(sample);
}

TEST_CASE("ScopedVoice frees its voice and never shares it", "[audio]")
{
    ScopedVoice a;
    REQUIRE_FALSE(a.isValid());
    a.create();
    const auto rid = a.rid();
    REQUIRE(a.isValid());

    ScopedVoice copy(a);
    REQUIRE_FALSE(copy.isValid());
    REQUIRE(IsAlive(rid));

    ScopedVoice moved(std::move(a));
    REQUIRE(moved.rid() == rid);
    REQUIRE_FALSE(a.isValid());

    ScopedVoice assigned;
    assigned.create();
    const auto replaced = assigned.rid();
    assigned = std::move(moved);
    REQUIRE(assigned.rid() == rid);
    REQUIRE_FALSE(IsAlive(replaced));

    assigned = copy;
    REQUIRE_FALSE(assigned.isValid());
    REQUIRE_FALSE(IsAlive(rid));
}

TEST_CASE("an emitter plays its file through the AudioServer", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    REQUIRE_FALSE(emitter->isPlaying());

    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->play();
    REQUIRE(emitter->isPlaying());
    REQUIRE(Near(MixFrame().first, 0.5f));

    emitter->stop();
    REQUIRE_FALSE(emitter->isPlaying());
    REQUIRE(MixFrame().first == 0.0f);
}

TEST_CASE("play works before the first tick and applies pending settings", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setVolume(0.5f);
    emitter->setPan(-1.0f);
    emitter->getComponentMutable<AudioEmitter>().audio_path = test.wav("a.wav", 0.5f);
    emitter->play();

    const auto [left, right] = MixFrame();
    REQUIRE(Near(left, 0.25f));
    REQUIRE(right == 0.0f);
}

TEST_CASE("settings changed in the component reach the voice on the next tick", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->play();

    auto &component = emitter->getComponentMutable<AudioEmitter>();
    component.volume = 0.5f;
    component.pan = 1.0f;
    REQUIRE(Near(MixFrame().first, 0.5f));

    test.world.progress(0.016f);
    const auto [left, right] = MixFrame();
    REQUIRE(left == 0.0f);
    REQUIRE(Near(right, 0.25f));
}

TEST_CASE("pitch and loop reach the voice", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("short.wav", 0.5f, 10));
    emitter->setPitch(2.0f);
    emitter->play();

    std::vector<float> out(40);
    AudioServer::Instance().mix(out.data(), 20);
    REQUIRE(out[8] != 0.0f);
    REQUIRE(out[10] == 0.0f);
    REQUIRE_FALSE(emitter->isPlaying());

    emitter->setLoop(true);
    emitter->play();
    AudioServer::Instance().mix(out.data(), 20);
    REQUIRE(out[38] != 0.0f);
    REQUIRE(emitter->isPlaying());
}

TEST_CASE("pausing an emitter keeps it playing but silent", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->play();

    emitter->setPaused(true);
    REQUIRE(MixFrame().first == 0.0f);
    REQUIRE(emitter->isPlaying());
    emitter->setPaused(false);
    REQUIRE(Near(MixFrame().first, 0.5f));
}

TEST_CASE("play restarts an emitter that is already playing", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("short.wav", 0.5f, 4));
    emitter->play();
    for (int i = 0; i < 3; ++i) MixFrame();

    emitter->play();
    std::vector<float> out(8);
    AudioServer::Instance().mix(out.data(), 4);
    REQUIRE(out[6] != 0.0f);
}

TEST_CASE("changing the path stops the old sound and keeps the settings", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setVolume(0.5f);
    const std::string first = test.wav("a.wav", 0.5f);
    emitter->setAudioPath(first);

    auto other = test.create();
    other->setAudioPath(first);
    emitter->play();
    emitter->setAudioPath(test.wav("b.wav", 0.25f));
    REQUIRE_FALSE(emitter->isPlaying());
    REQUIRE(MixFrame().first == 0.0f);

    emitter->play();
    REQUIRE(Near(MixFrame().first, 0.125f));

    emitter->setAudioPath("");
    REQUIRE_FALSE(emitter->isPlaying());
    emitter->play();
    REQUIRE_FALSE(emitter->isPlaying());
}

TEST_CASE("a missing file leaves the emitter silent and is not retried", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    const std::string path = (test.root / "later.wav").generic_string();
    emitter->setAudioPath(path);
    emitter->play();
    REQUIRE_FALSE(emitter->isPlaying());

    test.wav("later.wav", 0.5f);
    test.world.progress(0.016f);
    REQUIRE(emitter->getComponent<AudioEmitter>().m_sample == nullptr);
}

TEST_CASE("autoplay starts once in a game", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAutoplay(true);
    test.world.progress(0.016f);
    REQUIRE_FALSE(emitter->isPlaying());

    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    test.world.progress(0.016f);
    REQUIRE(emitter->isPlaying());

    emitter->stop();
    test.world.progress(0.016f);
    REQUIRE_FALSE(emitter->isPlaying());
}

TEST_CASE("autoplay never starts in the editor, but play does", "[audio][ecs]")
{
    TestWorld test(true);
    auto emitter = test.create();
    emitter->setAutoplay(true);
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    test.world.progress(0.016f);
    REQUIRE_FALSE(emitter->isPlaying());

    emitter->play();
    REQUIRE(emitter->isPlaying());
}

TEST_CASE("autoplay does not restart a sound already started by hand", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("short.wav", 0.5f, 4));
    emitter->setAutoplay(true);
    emitter->play();
    for (int i = 0; i < 3; ++i) MixFrame();

    test.world.progress(0.016f);
    MixFrame();
    MixFrame();
    REQUIRE_FALSE(emitter->isPlaying());
}

TEST_CASE("destroying an emitter frees its voice and its sample", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->setLoop(true);
    emitter->play();
    const auto voice = emitter->getComponent<AudioEmitter>().m_voice.rid();
    const auto sample = emitter->getComponent<AudioEmitter>().m_sample->rid();

    emitter->getHandle().destruct();
    REQUIRE(AudioServer::Instance().sampleGetFrames(sample) == 0);
    REQUIRE_FALSE(IsAlive(voice));
    REQUIRE(MixFrame().first == 0.0f);
}

TEST_CASE("destroying the world frees the voices", "[audio][ecs]")
{
    atmo::server::audio::VoiceRid voice;
    {
        TestWorld test;
        auto emitter = test.create();
        emitter->setAudioPath(test.wav("a.wav", 0.5f));
        emitter->play();
        voice = emitter->getComponent<AudioEmitter>().m_voice.rid();
    }
    REQUIRE_FALSE(IsAlive(voice));
    REQUIRE(MixFrame().first == 0.0f);
}

TEST_CASE("replacing the component does not leave the old voice playing", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->setLoop(true);
    emitter->play();
    const auto voice = emitter->getComponent<AudioEmitter>().m_voice.rid();

    emitter->setComponent<AudioEmitter>({});
    REQUIRE_FALSE(IsAlive(voice));
    REQUIRE(MixFrame().first == 0.0f);
}

TEST_CASE("a copied component gets its own voice and the same settings", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->setVolume(0.5f);
    test.world.progress(0.016f);

    auto copy = test.create();
    copy->setComponent(AudioEmitter(emitter->getComponent<AudioEmitter>()));
    copy->play();
    REQUIRE(copy->isPlaying());
    REQUIRE_FALSE(emitter->isPlaying());
    REQUIRE(copy->getComponent<AudioEmitter>().m_voice.rid() != emitter->getComponent<AudioEmitter>().m_voice.rid());
    REQUIRE(Near(MixFrame().first, 0.25f));

    copy->getHandle().destruct();
    emitter->play();
    REQUIRE(Near(MixFrame().first, 0.25f));
}

TEST_CASE("a NaN setting is ignored and recovers", "[audio][ecs]")
{
    TestWorld test;
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->play();

    emitter->setVolume(std::numeric_limits<float>::quiet_NaN());
    test.world.progress(0.016f);
    REQUIRE(Near(MixFrame().first, 0.5f));

    emitter->setVolume(0.5f);
    test.world.progress(0.016f);
    REQUIRE(Near(MixFrame().first, 0.25f));
}

TEST_CASE("an emitter saved in a scene plays once loaded", "[audio][ecs]")
{
    using atmo::core::ecs::entities::Entity;
    using atmo::core::ecs::entities::EntityData;

    TestWorld test;
    const std::string path = test.wav("music.wav", 0.5f);

    std::string json;
    {
        TestWorld::Editor editor;
        auto parent = EntityRegistry::CreateIn(&editor.world, "Entity");
        auto emitter = EntityRegistry::CreateIn<Emitter>(&editor.world, "Entity::AudioEmitter");
        parent->rename("Player");
        emitter->rename("Music");
        emitter->setParent(*parent);
        emitter->setVolume(0.5f);
        emitter->setLoop(true);
        emitter->setAutoplay(true);
        emitter->setAudioPath(path);
        editor.world.progress(0.016f);
        REQUIRE_FALSE(emitter->isPlaying());
        json = glz::write_json(parent->serialize()).value();
    }
    REQUIRE(json.find("music.wav") != std::string::npos);
    REQUIRE(json.find("m_voice") == std::string::npos);

    EntityData data;
    REQUIRE_FALSE(glz::read_json(data, json));
    auto loaded = EntityRegistry::CreateIn(&test.world, "Entity");
    loaded->deserializeInWorld(data, &test.world);
    test.world.progress(0.016f);

    Emitter emitter(loaded->getChild("Music").getHandle());
    REQUIRE(emitter.getLoop());
    REQUIRE(emitter.isPlaying());
    REQUIRE(Near(MixFrame().first, 0.25f));

    loaded->getHandle().destruct();
    REQUIRE(MixFrame().first == 0.0f);
}

TEST_CASE("an emitter plays in its bus", "[audio][ecs][bus]")
{
    TestWorld test;
    ScopedBusVolume music(Bus::Music, 0.5f);
    auto emitter = test.create();
    REQUIRE(emitter->getBus() == Bus::Master);
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->play();
    REQUIRE(Near(MixFrame().first, 0.5f));

    emitter->setBus(Bus::Music);
    REQUIRE(emitter->getBus() == Bus::Music);
    REQUIRE(emitter->getComponent<AudioEmitter>().bus == "music");
    test.world.progress(0.016f);
    REQUIRE(Near(MixFrame().first, 0.25f));

    emitter->setBus(Bus::Master);
    test.world.progress(0.016f);
    REQUIRE(Near(MixFrame().first, 0.5f));
}

TEST_CASE("the bus is applied by play before the first tick", "[audio][ecs][bus]")
{
    TestWorld test;
    ScopedBusVolume sfx(Bus::Sfx, 0.5f);
    auto emitter = test.create();
    emitter->setBus(Bus::Sfx);
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->play();
    REQUIRE(Near(MixFrame().first, 0.25f));
}

TEST_CASE("an unknown bus falls back to Master and recovers", "[audio][ecs][bus]")
{
    TestWorld test;
    ScopedBusVolume music(Bus::Music, 0.5f);
    auto emitter = test.create();
    emitter->setBus(Bus::Music);
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->play();
    REQUIRE(Near(MixFrame().first, 0.25f));

    emitter->getComponentMutable<AudioEmitter>().bus = "ambience";
    test.world.progress(0.016f);
    REQUIRE(emitter->getBus() == Bus::Master);
    REQUIRE(Near(MixFrame().first, 0.5f));

    emitter->setBus(Bus::Music);
    test.world.progress(0.016f);
    REQUIRE(Near(MixFrame().first, 0.25f));
}

TEST_CASE("an unknown bus in a scene does not lose the other settings", "[audio][ecs][bus]")
{
    const auto *info = MetaRegistry::Instance().find("AudioEmitter");
    REQUIRE(info != nullptr);

    AudioEmitter emitter;
    REQUIRE(info->from_json(&emitter, R"({"audio_path":"a.wav","bus":"ambience","loop":true,"volume":0.25})"));
    REQUIRE(emitter.audio_path == "a.wav");
    REQUIRE(emitter.loop);
    REQUIRE(emitter.volume == 0.25f);

    AudioEmitter old;
    REQUIRE(info->from_json(&old, R"({"audio_path":"a.wav","volume":0.25})"));
    REQUIRE(old.bus == "master");
}

TEST_CASE("a copied component keeps its bus", "[audio][ecs][bus]")
{
    TestWorld test;
    ScopedBusVolume music(Bus::Music, 0.5f);
    auto emitter = test.create();
    emitter->setAudioPath(test.wav("a.wav", 0.5f));
    emitter->setBus(Bus::Music);
    test.world.progress(0.016f);

    auto copy = test.create();
    copy->setComponent(AudioEmitter(emitter->getComponent<AudioEmitter>()));
    copy->play();
    REQUIRE(Near(MixFrame().first, 0.25f));
}
