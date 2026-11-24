#include <memory>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "core/resource/audio_sample.hpp"
#include "server/audio/audio_server.hpp"

using atmo::core::resource::AudioSample;
using atmo::server::audio::AudioServer;
using atmo::server::audio::SampleRid;
using Catch::Matchers::WithinAbs;

TEST_CASE("AudioSample cannot be copied", "[audio][resource]")
{
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<AudioSample>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<AudioSample>);
}

TEST_CASE("AudioSample exposes its RID and duration", "[audio][resource]")
{
    const SampleRid rid = AudioServer::Instance().sampleCreate(std::vector<float>(24000 * AudioServer::channels));
    AudioSample sample(rid);

    REQUIRE(sample.rid() == rid);
    REQUIRE_THAT(sample.duration(), WithinAbs(0.5, 1e-9));
}

TEST_CASE("AudioSample frees its sample when the last owner is gone", "[audio][resource]")
{
    auto &server = AudioServer::Instance();
    const SampleRid rid = server.sampleCreate({ 0.1f, 0.1f });
    const auto voice = server.voiceCreate();
    server.voiceSetSample(voice, rid);
    server.voicePlay(voice);

    auto first = std::make_shared<AudioSample>(rid);
    auto second = first;

    first.reset();
    REQUIRE(server.sampleGetFrames(rid) == 1);
    REQUIRE(server.voiceIsPlaying(voice));

    second.reset();
    REQUIRE(server.sampleGetFrames(rid) == 0);
    REQUIRE_FALSE(server.voiceIsPlaying(voice));
    server.voiceFree(voice);
}

TEST_CASE("AudioSample accepts an invalid RID", "[audio][resource]")
{
    AudioSample sample{ SampleRid{} };

    REQUIRE_FALSE(sample.rid().isValid());
    REQUIRE(sample.duration() == 0.0);
}
