#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "server/audio/audio_server.hpp"

using namespace atmo::server::audio;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr float tolerance = 1e-6f;

    AudioServer &Server()
    {
        return AudioServer::Instance();
    }

    struct ScopedSample {
        SampleRid rid;

        explicit ScopedSample(std::vector<float> pcm) : rid(Server().sampleCreate(std::move(pcm))) {}
        ~ScopedSample()
        {
            if (Server().sampleGetFrames(rid) != 0)
                Server().sampleFree(rid);
        }

        ScopedSample(const ScopedSample &) = delete;
        ScopedSample &operator=(const ScopedSample &) = delete;
    };

    struct ScopedVoice {
        VoiceRid rid;

        ScopedVoice() : rid(Server().voiceCreate()) {}
        explicit ScopedVoice(const ScopedSample &sample) : ScopedVoice()
        {
            Server().voiceSetSample(rid, sample.rid);
        }
        ~ScopedVoice()
        {
            Server().voiceFree(rid);
        }

        ScopedVoice(const ScopedVoice &) = delete;
        ScopedVoice &operator=(const ScopedVoice &) = delete;
    };

    struct ScopedBusVolumes {
        ScopedBusVolumes() = default;
        ~ScopedBusVolumes()
        {
            for (const Bus bus : { Bus::Master, Bus::Music, Bus::Sfx }) Server().busSetVolume(bus, 1.0f);
        }

        ScopedBusVolumes(const ScopedBusVolumes &) = delete;
        ScopedBusVolumes &operator=(const ScopedBusVolumes &) = delete;
    };

    struct ScopedLimiter {
        ScopedLimiter() = default;
        ~ScopedLimiter()
        {
            Settle();
        }

        ScopedLimiter(const ScopedLimiter &) = delete;
        ScopedLimiter &operator=(const ScopedLimiter &) = delete;

        static void Settle()
        {
            std::vector<float> out(AudioServer::sample_rate * 2 * AudioServer::channels);
            Server().mix(out.data(), AudioServer::sample_rate * 2);
        }
    };

    std::vector<float> Stereo(const std::vector<float> &mono)
    {
        std::vector<float> pcm;
        for (float value : mono) {
            pcm.push_back(value);
            pcm.push_back(value);
        }
        return pcm;
    }

    std::vector<float> Mix(int frames)
    {
        std::vector<float> out(static_cast<std::size_t>(frames) * AudioServer::channels, 42.0f);
        Server().mix(out.data(), frames);
        return out;
    }

    void RequireEqual(const std::vector<float> &actual, const std::vector<float> &expected)
    {
        REQUIRE(actual.size() == expected.size());
        for (std::size_t i = 0; i < actual.size(); ++i) {
            INFO("float " << i);
            REQUIRE_THAT(actual[i], WithinAbs(expected[i], tolerance));
        }
    }
} // namespace

TEST_CASE("sampleCreate rejects data that is not stereo", "[audio]")
{
    REQUIRE_FALSE(Server().sampleCreate({ 0.1f }).isValid());
    REQUIRE_FALSE(Server().sampleCreate({ 0.1f, 0.2f, 0.3f }).isValid());
}

TEST_CASE("sampleGetFrames counts frames and ignores dead samples", "[audio]")
{
    ScopedSample sample({ 0.1f, 0.2f, 0.3f, 0.4f });
    REQUIRE(sample.rid.isValid());
    REQUIRE(Server().sampleGetFrames(sample.rid) == 2);

    Server().sampleFree(sample.rid);
    REQUIRE(Server().sampleGetFrames(sample.rid) == 0);
    REQUIRE(Server().sampleGetFrames({}) == 0);
}

TEST_CASE("mix outputs silence when nothing plays", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.5f, 0.5f }));
    ScopedVoice voice(sample);

    RequireEqual(Mix(2), Stereo({ 0.0f, 0.0f }));
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));
}

TEST_CASE("a voice plays both channels then stops at the end", "[audio][mix]")
{
    ScopedSample sample({ 0.1f, -0.1f, 0.2f, -0.2f });
    ScopedVoice voice(sample);
    Server().voicePlay(voice.rid);
    REQUIRE(Server().voiceIsPlaying(voice.rid));

    RequireEqual(Mix(4), { 0.1f, -0.1f, 0.2f, -0.2f, 0.0f, 0.0f, 0.0f, 0.0f });
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));
}

TEST_CASE("a voice continues where the previous mix stopped", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f, 0.4f }));
    ScopedVoice voice(sample);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(1), Stereo({ 0.1f }));
    RequireEqual(Mix(2), Stereo({ 0.2f, 0.3f }));
    RequireEqual(Mix(2), Stereo({ 0.4f, 0.0f }));
}

TEST_CASE("voicePlay restarts a voice that is already playing", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f }));
    ScopedVoice voice(sample);
    Server().voicePlay(voice.rid);
    Mix(2);

    Server().voicePlay(voice.rid);
    RequireEqual(Mix(2), Stereo({ 0.1f, 0.2f }));
}

TEST_CASE("a looping voice wraps around, inside a mix and across mixes", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f }));
    ScopedVoice voice(sample);
    Server().voiceSetLoop(voice.rid, true);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(4), Stereo({ 0.1f, 0.2f, 0.3f, 0.1f }));
    RequireEqual(Mix(3), Stereo({ 0.2f, 0.3f, 0.1f }));
    REQUIRE(Server().voiceIsPlaying(voice.rid));

    Server().voiceSetLoop(voice.rid, false);
    RequireEqual(Mix(3), Stereo({ 0.2f, 0.3f, 0.0f }));
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));
}

TEST_CASE("looping keeps the fractional overshoot and blends the seam", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.0f, 0.4f }));
    ScopedVoice voice(sample);
    Server().voiceSetLoop(voice.rid, true);
    Server().voiceSetPitch(voice.rid, 1.5f);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(4), Stereo({ 0.0f, 0.2f, 0.4f, 0.2f }));
}

TEST_CASE("looping survives a pitch longer than the sample", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f }));
    ScopedVoice voice(sample);
    Server().voiceSetLoop(voice.rid, true);
    Server().voiceSetPitch(voice.rid, 5.0f);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(4), Stereo({ 0.1f, 0.2f, 0.1f, 0.2f }));
}

TEST_CASE("pausing freezes a voice and resuming continues it", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f }));
    ScopedVoice voice(sample);
    Server().voicePlay(voice.rid);
    Mix(1);

    Server().voiceSetPaused(voice.rid, true);
    RequireEqual(Mix(2), Stereo({ 0.0f, 0.0f }));
    REQUIRE(Server().voiceIsPlaying(voice.rid));

    Server().voiceSetPaused(voice.rid, false);
    RequireEqual(Mix(2), Stereo({ 0.2f, 0.3f }));
}

TEST_CASE("stopping silences a voice and rewinds it", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f }));
    ScopedVoice voice(sample);
    Server().voicePlay(voice.rid);
    Mix(1);

    Server().voiceStop(voice.rid);
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));
    RequireEqual(Mix(2), Stereo({ 0.0f, 0.0f }));

    Server().voicePlay(voice.rid);
    RequireEqual(Mix(1), Stereo({ 0.1f }));
}

TEST_CASE("volume scales the output and cannot be negative", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.4f, 0.4f }));
    ScopedVoice voice(sample);
    Server().voicePlay(voice.rid);

    Server().voiceSetVolume(voice.rid, 0.5f);
    RequireEqual(Mix(1), Stereo({ 0.2f }));
    Server().voiceSetVolume(voice.rid, -1.0f);
    RequireEqual(Mix(1), Stereo({ 0.0f }));
}

TEST_CASE("pitch above one skips frames and shortens the sound", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f, 0.4f }));
    ScopedVoice voice(sample);
    Server().voiceSetPitch(voice.rid, 2.0f);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(3), Stereo({ 0.1f, 0.3f, 0.0f }));
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));
}

TEST_CASE("pitch below one interpolates and lengthens the sound", "[audio][mix]")
{
    ScopedSample sample({ 0.0f, 0.4f, 0.4f, 0.0f });
    ScopedVoice voice(sample);
    Server().voiceSetPitch(voice.rid, 0.25f);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(9), { 0.0f, 0.4f, 0.1f, 0.3f, 0.2f, 0.2f, 0.3f, 0.1f, 0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f });
}

TEST_CASE("a zero or negative pitch still moves forward", "[audio][mix]")
{
    const float pitch = GENERATE(0.0f, -3.0f);
    ScopedSample sample(Stereo({ 0.5f }));
    ScopedVoice voice(sample);
    Server().voiceSetPitch(voice.rid, pitch);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(1), Stereo({ 0.5f }));
    Mix(200);
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));
}

TEST_CASE("pan lowers the opposite side only", "[audio][mix]")
{
    struct Case {
        float pan;
        float left;
        float right;
    };
    const Case c = GENERATE(
        Case{ 0.0f, 0.4f, 0.4f },
        Case{ -1.0f, 0.4f, 0.0f },
        Case{ 1.0f, 0.0f, 0.4f },
        Case{ -0.5f, 0.4f, 0.2f },
        Case{ 0.5f, 0.2f, 0.4f },
        Case{ -7.0f, 0.4f, 0.0f },
        Case{ 7.0f, 0.0f, 0.4f });

    ScopedSample sample(Stereo({ 0.4f }));
    ScopedVoice voice(sample);
    Server().voiceSetPan(voice.rid, c.pan);
    Server().voicePlay(voice.rid);

    INFO("pan " << c.pan);
    RequireEqual(Mix(1), { c.left, c.right });
}

TEST_CASE("voices are summed and keep their own settings", "[audio][mix]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f }));
    ScopedVoice a(sample);
    ScopedVoice b(sample);
    Server().voicePlay(a.rid);
    Mix(1);

    Server().voiceSetVolume(b.rid, 0.5f);
    Server().voicePlay(b.rid);

    RequireEqual(Mix(3), Stereo({ 0.25f, 0.4f, 0.15f }));
}

TEST_CASE("the mix is limited after summing, not before", "[audio][mix]")
{
    ScopedLimiter reset;
    ScopedSample loud(Stereo({ 0.8f, -0.8f }));
    ScopedSample opposite(Stereo({ 0.0f, 0.5f }));
    ScopedVoice a(loud);
    ScopedVoice b(loud);
    ScopedVoice c(opposite);
    for (const VoiceRid voice : { a.rid, b.rid, c.rid }) Server().voicePlay(voice);

    const auto out = Mix(2);
    REQUIRE(out[0] == 1.0f);
    REQUIRE(out[1] == 1.0f);
    REQUIRE_THAT(out[2], WithinAbs(-1.1f / 1.6f, 1e-3f));

    ScopedLimiter::Settle();
    ScopedSample inverse(Stereo({ -0.8f }));
    ScopedVoice d(inverse);
    for (const VoiceRid voice : { a.rid, d.rid }) Server().voicePlay(voice);
    RequireEqual(Mix(1), Stereo({ 0.0f }));
}

TEST_CASE("freeing a sample stops the voices using it and no others", "[audio]")
{
    ScopedSample freed(Stereo({ 0.1f, 0.1f }));
    ScopedSample kept(Stereo({ 0.2f, 0.2f }));
    ScopedVoice a(freed);
    ScopedVoice b(kept);
    Server().voicePlay(a.rid);
    Server().voicePlay(b.rid);

    Server().sampleFree(freed.rid);

    REQUIRE_FALSE(Server().voiceIsPlaying(a.rid));
    REQUIRE(Server().voiceIsPlaying(b.rid));
    RequireEqual(Mix(1), Stereo({ 0.2f }));

    ScopedSample reused(Stereo({ 0.7f }));
    REQUIRE(reused.rid.slot() == freed.rid.slot());
    Server().voicePlay(a.rid);
    REQUIRE_FALSE(Server().voiceIsPlaying(a.rid));
}

TEST_CASE("a voice without a live sample does not play", "[audio]")
{
    ScopedVoice voice;
    Server().voicePlay(voice.rid);
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));

    ScopedSample sample(Stereo({ 0.5f }));
    Server().voiceSetSample(voice.rid, sample.rid);
    Server().voicePlay(voice.rid);
    Server().voiceSetSample(voice.rid, {});
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));

    Server().sampleFree(sample.rid);
    Server().voiceSetSample(voice.rid, sample.rid);
    Server().voicePlay(voice.rid);
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));
}

TEST_CASE("an empty sample stops instead of playing", "[audio][mix]")
{
    const SampleRid sample = Server().sampleCreate({});
    ScopedVoice voice;
    Server().voiceSetSample(voice.rid, sample);
    Server().voiceSetLoop(voice.rid, true);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(2), Stereo({ 0.0f, 0.0f }));
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));
    Server().sampleFree(sample);
}

TEST_CASE("calls on a freed voice never reach the voice that reused its slot", "[audio]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f }));
    const VoiceRid freed = Server().voiceCreate();
    Server().voiceFree(freed);

    ScopedVoice voice(sample);
    REQUIRE(voice.rid.slot() == freed.slot());
    Server().voicePlay(voice.rid);

    Server().voiceSetVolume(freed, 0.0f);
    Server().voiceSetPitch(freed, 2.0f);
    Server().voiceSetPan(freed, 1.0f);
    Server().voiceSetPaused(freed, true);
    Server().voiceSetSample(freed, {});
    Server().voiceStop(freed);
    Server().voiceFree(freed);

    REQUIRE_FALSE(Server().voiceIsPlaying(freed));
    REQUIRE(Server().voiceIsPlaying(voice.rid));
    RequireEqual(Mix(2), Stereo({ 0.1f, 0.2f }));
}

TEST_CASE("non-finite settings are ignored and keep the previous value", "[audio]")
{
    constexpr float infinity = std::numeric_limits<float>::infinity();
    constexpr float nan = std::numeric_limits<float>::quiet_NaN();
    const float bad = GENERATE_COPY(nan, -nan, infinity, -infinity);

    ScopedSample sample(Stereo({ 0.2f, 0.4f, 0.6f, 0.8f }));
    ScopedVoice voice(sample);
    Server().voiceSetVolume(voice.rid, 0.5f);
    Server().voiceSetPitch(voice.rid, 2.0f);
    Server().voiceSetPan(voice.rid, -1.0f);

    Server().voiceSetVolume(voice.rid, bad);
    Server().voiceSetPitch(voice.rid, bad);
    Server().voiceSetPan(voice.rid, bad);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(2), { 0.1f, 0.0f, 0.3f, 0.0f });
}

TEST_CASE("bus names round-trip and unknown ones are rejected", "[audio][bus]")
{
    const Bus bus = GENERATE(Bus::Master, Bus::Music, Bus::Sfx);
    REQUIRE(ParseBus(BusName(bus)) == bus);

    REQUIRE(BusName(Bus::Master) == "master");
    REQUIRE(BusName(Bus::Music) == "music");
    REQUIRE(BusName(Bus::Sfx) == "sfx");
    REQUIRE(BusName(static_cast<Bus>(200)).empty());
    REQUIRE_FALSE(ParseBus("Music").has_value());
    REQUIRE_FALSE(ParseBus("voice").has_value());
    REQUIRE_FALSE(ParseBus("").has_value());
}

TEST_CASE("every bus reaches the output untouched by default", "[audio][mix][bus]")
{
    const Bus bus = GENERATE(Bus::Master, Bus::Music, Bus::Sfx);
    ScopedSample sample({ 0.1f, -0.1f, 0.2f, -0.2f });
    ScopedVoice voice(sample);
    Server().voiceSetBus(voice.rid, bus);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(3), { 0.1f, -0.1f, 0.2f, -0.2f, 0.0f, 0.0f });
}

TEST_CASE("a bus volume only affects its own voices", "[audio][mix][bus]")
{
    struct Case {
        Bus bus;
        float expected;
    };
    const Case c = GENERATE(Case{ Bus::Music, 0.6f }, Case{ Bus::Sfx, 0.5f }, Case{ Bus::Master, 0.35f });

    ScopedBusVolumes reset;
    ScopedSample a(Stereo({ 0.1f }));
    ScopedSample b(Stereo({ 0.2f }));
    ScopedSample d(Stereo({ 0.4f }));
    ScopedVoice master(a);
    ScopedVoice music(b);
    ScopedVoice sfx(d);
    Server().voiceSetBus(music.rid, Bus::Music);
    Server().voiceSetBus(sfx.rid, Bus::Sfx);
    for (const VoiceRid voice : { master.rid, music.rid, sfx.rid }) Server().voicePlay(voice);

    Server().busSetVolume(c.bus, 0.5f);
    INFO("bus " << BusName(c.bus));
    RequireEqual(Mix(1), Stereo({ c.expected }));
}

TEST_CASE("voice, bus and Master volumes multiply", "[audio][mix][bus]")
{
    ScopedBusVolumes reset;
    ScopedSample sample(Stereo({ 0.8f }));
    ScopedVoice voice(sample);
    Server().voiceSetBus(voice.rid, Bus::Music);
    Server().voiceSetVolume(voice.rid, 0.5f);
    Server().voiceSetPan(voice.rid, -0.5f);
    Server().busSetVolume(Bus::Music, 0.5f);
    Server().busSetVolume(Bus::Master, 0.5f);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(1), { 0.1f, 0.05f });
}

TEST_CASE("a bus can be turned up and its volume cannot be negative", "[audio][mix][bus]")
{
    ScopedBusVolumes reset;
    ScopedSample sample(Stereo({ 0.2f, 0.2f }));
    ScopedVoice voice(sample);
    Server().voiceSetBus(voice.rid, Bus::Sfx);
    Server().voicePlay(voice.rid);

    Server().busSetVolume(Bus::Sfx, 2.0f);
    RequireEqual(Mix(1), Stereo({ 0.4f }));
    REQUIRE(Server().busGetVolume(Bus::Sfx) == 2.0f);
    Server().busSetVolume(Bus::Sfx, -1.0f);
    REQUIRE(Server().busGetVolume(Bus::Sfx) == 0.0f);
    RequireEqual(Mix(1), Stereo({ 0.0f }));
}

TEST_CASE("a muted bus keeps its voices moving", "[audio][mix][bus]")
{
    ScopedBusVolumes reset;
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f }));
    ScopedVoice voice(sample);
    Server().voiceSetBus(voice.rid, Bus::Music);
    Server().voicePlay(voice.rid);

    Server().busSetVolume(Bus::Music, 0.0f);
    RequireEqual(Mix(2), Stereo({ 0.0f, 0.0f }));
    REQUIRE(Server().voiceIsPlaying(voice.rid));
    Server().busSetVolume(Bus::Music, 1.0f);
    RequireEqual(Mix(1), Stereo({ 0.3f }));
}

TEST_CASE("the limiter applies after the bus volumes", "[audio][mix][bus]")
{
    ScopedBusVolumes reset;
    ScopedSample sample(Stereo({ 0.8f, 0.8f }));
    ScopedVoice a(sample);
    ScopedVoice b(sample);
    for (const VoiceRid voice : { a.rid, b.rid }) {
        Server().voiceSetBus(voice, Bus::Sfx);
        Server().voicePlay(voice);
    }

    Server().busSetVolume(Bus::Sfx, 0.5f);
    RequireEqual(Mix(1), Stereo({ 0.8f }));
    Server().busSetVolume(Bus::Sfx, 1.0f);
    Server().busSetVolume(Bus::Master, 0.5f);
    RequireEqual(Mix(1), Stereo({ 0.8f }));
}

TEST_CASE("a voice changes bus while playing", "[audio][mix][bus]")
{
    ScopedBusVolumes reset;
    ScopedSample sample(Stereo({ 0.2f, 0.4f, 0.6f }));
    ScopedVoice voice(sample);
    Server().busSetVolume(Bus::Music, 0.5f);
    Server().busSetVolume(Bus::Sfx, 0.0f);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(1), Stereo({ 0.2f }));
    Server().voiceSetBus(voice.rid, Bus::Music);
    RequireEqual(Mix(1), Stereo({ 0.2f }));
    Server().voiceSetBus(voice.rid, Bus::Sfx);
    RequireEqual(Mix(1), Stereo({ 0.0f }));
}

TEST_CASE("a voice reusing a slot starts back in Master", "[audio][bus]")
{
    ScopedBusVolumes reset;
    Server().busSetVolume(Bus::Music, 0.0f);
    const VoiceRid freed = Server().voiceCreate();
    Server().voiceSetBus(freed, Bus::Music);
    Server().voiceFree(freed);

    ScopedSample sample(Stereo({ 0.2f }));
    ScopedVoice voice(sample);
    REQUIRE(voice.rid.slot() == freed.slot());
    Server().voiceSetBus(freed, Bus::Music);
    Server().voicePlay(voice.rid);
    RequireEqual(Mix(1), Stereo({ 0.2f }));
}

TEST_CASE("bad bus arguments are ignored", "[audio][bus]")
{
    constexpr float infinity = std::numeric_limits<float>::infinity();
    const float bad = GENERATE_COPY(std::numeric_limits<float>::quiet_NaN(), infinity, -infinity);

    ScopedBusVolumes reset;
    ScopedSample sample(Stereo({ 0.4f }));
    ScopedVoice voice(sample);
    Server().voiceSetBus(voice.rid, Bus::Music);
    Server().busSetVolume(Bus::Music, 0.5f);

    Server().busSetVolume(Bus::Music, bad);
    Server().busSetVolume(Bus::Master, bad);
    Server().busSetVolume(static_cast<Bus>(200), 0.0f);
    Server().voiceSetBus(voice.rid, static_cast<Bus>(200));
    REQUIRE(Server().busGetVolume(Bus::Music) == 0.5f);
    REQUIRE(Server().busGetVolume(Bus::Master) == 1.0f);
    REQUIRE(Server().busGetVolume(static_cast<Bus>(200)) == 0.0f);
    Server().voicePlay(voice.rid);
    RequireEqual(Mix(1), Stereo({ 0.2f }));
}

TEST_CASE("buses hold when the number of frames changes between mixes", "[audio][mix][bus]")
{
    ScopedBusVolumes reset;
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f }));
    ScopedVoice voice(sample);
    Server().voiceSetBus(voice.rid, Bus::Sfx);
    Server().voicePlay(voice.rid);

    RequireEqual(Mix(3), Stereo({ 0.1f, 0.2f, 0.3f }));
    RequireEqual(Mix(1), Stereo({ 0.4f }));
    RequireEqual(Mix(4), Stereo({ 0.5f, 0.6f, 0.0f, 0.0f }));
}

TEST_CASE("update ends voices when no device plays them", "[audio][update]")
{
    ScopedSample sample(std::vector<float>(AudioServer::sample_rate * AudioServer::channels, 0.5f));
    ScopedVoice voice(sample);
    Server().voicePlay(voice.rid);

    Server().update(0.5f);
    REQUIRE(Server().voiceIsPlaying(voice.rid));
    Server().update(0.25f);
    REQUIRE(Server().voiceIsPlaying(voice.rid));
    Server().update(0.25f);
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));

    Server().voicePlay(voice.rid);
    Server().update(0.5f);
    REQUIRE(Server().voiceIsPlaying(voice.rid));
}

TEST_CASE("update follows the pitch", "[audio][update]")
{
    ScopedSample sample(std::vector<float>(AudioServer::sample_rate * AudioServer::channels, 0.5f));
    ScopedVoice fast(sample);
    ScopedVoice slow(sample);
    Server().voiceSetPitch(fast.rid, 2.0f);
    Server().voiceSetPitch(slow.rid, 0.5f);
    Server().voicePlay(fast.rid);
    Server().voicePlay(slow.rid);

    Server().update(0.5f);
    REQUIRE_FALSE(Server().voiceIsPlaying(fast.rid));
    Server().update(1.0f);
    REQUIRE(Server().voiceIsPlaying(slow.rid));
    Server().update(0.5f);
    REQUIRE_FALSE(Server().voiceIsPlaying(slow.rid));
}

TEST_CASE("update leaves paused and stopped voices alone", "[audio][update]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f }));
    ScopedVoice paused(sample);
    ScopedVoice stopped(sample);
    Server().voicePlay(paused.rid);
    Server().voiceSetPaused(paused.rid, true);

    Server().update(10.0f);
    REQUIRE(Server().voiceIsPlaying(paused.rid));
    REQUIRE_FALSE(Server().voiceIsPlaying(stopped.rid));
    Server().voiceSetPaused(paused.rid, false);
    RequireEqual(Mix(1), Stereo({ 0.1f }));
}

TEST_CASE("update wraps looping voices", "[audio][update]")
{
    ScopedSample sample(Stereo({ 0.1f, 0.2f, 0.3f, 0.4f }));
    ScopedVoice voice(sample);
    Server().voiceSetLoop(voice.rid, true);
    Server().voicePlay(voice.rid);

    Server().update(6.0f / AudioServer::sample_rate);
    REQUIRE(Server().voiceIsPlaying(voice.rid));
    const std::vector<float> out = Mix(1);
    REQUIRE_THAT(out[0], WithinAbs(0.3f, 1e-4f));
    REQUIRE_THAT(out[1], WithinAbs(0.3f, 1e-4f));
}

TEST_CASE("update ignores bad delta times", "[audio][update]")
{
    constexpr float infinity = std::numeric_limits<float>::infinity();
    const float bad = GENERATE_COPY(0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(), infinity, -infinity);

    ScopedSample sample(Stereo({ 0.1f, 0.2f }));
    ScopedVoice voice(sample);
    Server().voicePlay(voice.rid);

    Server().update(bad);
    REQUIRE(Server().voiceIsPlaying(voice.rid));
    RequireEqual(Mix(1), Stereo({ 0.1f }));
}

TEST_CASE("update stops voices whose sample is empty", "[audio][update]")
{
    ScopedSample sample(std::vector<float>{});
    ScopedVoice voice(sample);
    Server().voicePlay(voice.rid);

    Server().update(0.1f);
    REQUIRE_FALSE(Server().voiceIsPlaying(voice.rid));
}
