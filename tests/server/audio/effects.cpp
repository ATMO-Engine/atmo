#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <numbers>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "server/audio/audio_server.hpp"
#include "server/audio/effects/delay.hpp"
#include "server/audio/effects/ducker.hpp"
#include "server/audio/effects/effect_kind.hpp"
#include "server/audio/effects/limiter.hpp"

using namespace atmo::server::audio;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr std::size_t second = AudioServer::sample_rate;

    AudioServer &Server()
    {
        return AudioServer::Instance();
    }

    // AudioServer is a singleton: nothing set here may leak into the next test.
    struct ScopedBuses {
        ScopedBuses() = default;
        ~ScopedBuses()
        {
            for (const Bus bus : { Bus::Master, Bus::Music, Bus::Sfx }) {
                Server().busSetVolume(bus, 1.0f);
                Server().busRemoveDucking(bus);
                for (const auto name : effect_names) Server().busRemoveEffect(bus, *ParseEffect(name));
            }
            std::vector<float> settle(second * 2 * 2);
            Server().mix(settle.data(), second * 2);
        }

        ScopedBuses(const ScopedBuses &) = delete;
        ScopedBuses &operator=(const ScopedBuses &) = delete;
    };

    struct PlayingVoice {
        SampleRid sample;
        VoiceRid voice;

        PlayingVoice(std::vector<float> pcm, Bus bus) : sample(Server().sampleCreate(std::move(pcm))), voice(Server().voiceCreate())
        {
            Server().voiceSetSample(voice, sample);
            Server().voiceSetBus(voice, bus);
            Server().voicePlay(voice);
        }
        ~PlayingVoice()
        {
            Server().voiceFree(voice);
            Server().sampleFree(sample);
        }

        PlayingVoice(const PlayingVoice &) = delete;
        PlayingVoice &operator=(const PlayingVoice &) = delete;
    };

    std::vector<float> Sine(float frequency, std::size_t frames = second, float amplitude = 0.5f)
    {
        std::vector<float> pcm;
        for (std::size_t i = 0; i < frames; ++i) {
            const double phase = 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / AudioServer::sample_rate;
            const float value = amplitude * static_cast<float>(std::sin(phase));
            pcm.insert(pcm.end(), { value, value });
        }
        return pcm;
    }

    std::vector<float> Constant(float value, std::size_t frames)
    {
        return std::vector<float>(frames * 2, value);
    }

    std::vector<float> Mix(std::size_t frames)
    {
        std::vector<float> out(frames * 2);
        Server().mix(out.data(), static_cast<int>(frames));
        return out;
    }

    float Peak(const std::vector<float> &pcm, std::size_t from_frame = 0)
    {
        float peak = 0.0f;
        for (std::size_t i = from_frame * 2; i < pcm.size(); ++i) peak = std::max(peak, std::abs(pcm[i]));
        return peak;
    }

    // Amplitude of a sine, even when its peaks fall between two frames
    float Amplitude(const std::vector<float> &pcm, std::size_t from_frame)
    {
        double sum = 0.0;
        for (std::size_t i = from_frame * 2; i < pcm.size(); ++i) sum += static_cast<double>(pcm[i]) * pcm[i];
        return static_cast<float>(std::sqrt(2.0 * sum / static_cast<double>(pcm.size() - from_frame * 2)));
    }

    std::unique_ptr<AudioEffect> Make(EffectKind kind, std::vector<EffectValue> changes = {})
    {
        std::vector<float> values;
        for (const auto &parameter : EffectParameters(kind)) {
            values.push_back(parameter.initial);
            for (const auto &change : changes) {
                if (change.name == parameter.name)
                    values.back() = change.value;
            }
        }
        auto effect = CreateEffect(kind);
        effect->setParameters(values);
        return effect;
    }

    // Gain of an effect for a sine, once it has settled, in decibels
    float Gain(AudioEffect &effect, float frequency)
    {
        auto pcm = Sine(frequency);
        effect.process(pcm.data(), second);
        return 20.0f * std::log10(Amplitude(pcm, second / 2) / 0.5f);
    }

    EffectKind AnyEffect()
    {
        return GENERATE(EffectKind::HighPass, EffectKind::LowPass, EffectKind::Eq3Band, EffectKind::Delay, EffectKind::Reverb);
    }
} // namespace

TEST_CASE("low_pass keeps the bass and cuts the treble", "[audio][effects]")
{
    const auto effect = Make(EffectKind::LowPass, { { "cutoff", 1000.0f } });
    REQUIRE_THAT(Gain(*effect, 100.0f), WithinAbs(0.0f, 0.1f));
    REQUIRE_THAT(Gain(*effect, 1000.0f), WithinAbs(-3.0f, 0.1f));
    REQUIRE(Gain(*effect, 8000.0f) < -30.0f);
}

TEST_CASE("high_pass keeps the treble and cuts the bass", "[audio][effects]")
{
    const auto effect = Make(EffectKind::HighPass, { { "cutoff", 1000.0f } });
    REQUIRE_THAT(Gain(*effect, 8000.0f), WithinAbs(0.0f, 0.1f));
    REQUIRE_THAT(Gain(*effect, 1000.0f), WithinAbs(-3.0f, 0.1f));
    REQUIRE(Gain(*effect, 100.0f) < -30.0f);
}

TEST_CASE("resonance boosts the cutoff", "[audio][effects]")
{
    const auto kind = GENERATE(EffectKind::LowPass, EffectKind::HighPass);
    const auto effect = Make(kind, { { "cutoff", 1000.0f }, { "resonance", 4.0f } });
    REQUIRE_THAT(Gain(*effect, 1000.0f), WithinAbs(12.04f, 0.1f));
}

TEST_CASE("eq changes each band on its own", "[audio][effects]")
{
    struct Case {
        std::string_view band;
        float frequency;
    };
    const Case c = GENERATE(Case{ "low", 40.0f }, Case{ "mid", 1000.0f }, Case{ "high", 16000.0f });
    const float gain = GENERATE(-12.0f, 6.0f);

    const auto effect = Make(EffectKind::Eq3Band, { { c.band, gain } });
    for (const float frequency : { 40.0f, 1000.0f, 16000.0f }) {
        INFO(c.band << " at " << gain << " dB, heard at " << frequency << " Hz");
        REQUIRE_THAT(Gain(*effect, frequency), WithinAbs(frequency == c.frequency ? gain : 0.0f, 0.5f));
    }
}

TEST_CASE("a flat eq does not change the sound", "[audio][effects]")
{
    const auto effect = Make(EffectKind::Eq3Band);
    const auto input = Sine(440.0f, 1000);
    auto output = input;
    effect->process(output.data(), 1000);
    for (std::size_t i = 0; i < input.size(); ++i) REQUIRE_THAT(output[i], WithinAbs(input[i], 1e-6f));
}

TEST_CASE("delay repeats the sound, fading each time", "[audio][effects]")
{
    const auto effect = Make(EffectKind::Delay, { { "time", 0.1f }, { "feedback", 0.5f }, { "mix", 0.5f } });
    auto pcm = Constant(0.0f, 15000);
    pcm[0] = 1.0f;
    pcm[1] = -0.5f;
    effect->process(pcm.data(), 15000);

    const std::size_t step = 4800;
    for (std::size_t i = 0; i < 15000; ++i) {
        float expected = 0.0f;
        if (i == 0)
            expected = 0.5f;
        else if (i % step == 0)
            expected = 0.5f * std::pow(0.5f, static_cast<float>(i / step - 1));
        INFO("frame " << i);
        REQUIRE_THAT(pcm[i * 2], WithinAbs(expected, 1e-6f));
        REQUIRE_THAT(pcm[i * 2 + 1], WithinAbs(-0.5f * expected, 1e-6f));
    }
}

TEST_CASE("delay reaches its longest and shortest times", "[audio][effects]")
{
    const float time = GENERATE(0.001f, Delay::max_time);
    const auto effect = Make(EffectKind::Delay, { { "time", time }, { "feedback", 0.0f }, { "mix", 1.0f } });
    const auto expected = static_cast<std::size_t>(std::lround(time * AudioServer::sample_rate));

    auto pcm = Constant(0.0f, expected + 10);
    pcm[0] = 1.0f;
    effect->process(pcm.data(), expected + 10);
    REQUIRE(pcm[expected * 2] == 1.0f);
    REQUIRE(pcm[0] == 0.0f);
}

TEST_CASE("reverb leaves a stereo tail that fades, longer in a bigger room", "[audio][effects]")
{
    const auto tail = [](float room_size) {
        const auto effect = Make(EffectKind::Reverb, { { "room_size", room_size }, { "mix", 1.0f } });
        auto pcm = Constant(0.0f, second * 2);
        pcm[0] = pcm[1] = 1.0f;
        effect->process(pcm.data(), second * 2);
        return pcm;
    };
    const auto energy = [](const std::vector<float> &pcm, std::size_t from, std::size_t to) {
        double sum = 0.0;
        for (std::size_t i = from * 2; i < to * 2; ++i) sum += static_cast<double>(pcm[i]) * pcm[i];
        return sum;
    };

    const auto small = tail(0.0f);
    const auto big = tail(1.0f);
    REQUIRE(energy(small, 0, second / 2) > 0.0);
    REQUIRE(energy(small, second, second * 2) < energy(small, 0, second / 2) * 1e-3);
    REQUIRE(energy(big, second, second * 2) > energy(small, second, second * 2) * 10.0);
    REQUIRE(energy(big, second, second * 2) < energy(big, 0, second));

    bool stereo = false;
    for (std::size_t i = 0; i < second; ++i) stereo = stereo || big[i * 2] != big[i * 2 + 1];
    REQUIRE(stereo);
}

TEST_CASE("effects stay bounded and fade to exact silence", "[audio][effects]")
{
    const EffectKind kind = AnyEffect();
    const bool high = GENERATE(false, true);
    INFO(effect_names[EffectIndex(kind)] << (high ? " at the highest settings" : " at the lowest settings"));

    std::vector<float> values;
    for (const auto &parameter : EffectParameters(kind)) values.push_back(high ? parameter.max : parameter.min);
    const auto effect = CreateEffect(kind);
    effect->setParameters(values);

    auto pcm = Sine(997.0f, second * 2);
    effect->process(pcm.data(), second * 2);
    const float peak = Peak(pcm);
    REQUIRE(std::isfinite(peak));
    REQUIRE(peak < 50.0f);

    for (int i = 0; i < 2000 && peak > 0.0f; ++i) {
        pcm = Constant(0.0f, second);
        effect->process(pcm.data(), second);
        if (Peak(pcm) == 0.0f)
            break;
    }
    REQUIRE(Peak(pcm) == 0.0f);
}

TEST_CASE("the result does not depend on the size of the blocks", "[audio][effects]")
{
    const EffectKind kind = AnyEffect();
    auto whole = Sine(440.0f, 6000);
    auto blocks = whole;
    Make(kind, { { "time", 0.01f } })->process(whole.data(), 6000);

    const auto effect = Make(kind, { { "time", 0.01f } });
    std::size_t done = 0;
    for (std::size_t block = 1; done < 6000; block = block * 3 % 701 + 1) {
        const std::size_t frames = std::min(block, 6000 - done);
        effect->process(blocks.data() + done * 2, frames);
        done += frames;
    }
    REQUIRE(whole == blocks);
}

TEST_CASE("the limiter leaves quiet sounds alone", "[audio][effects][limiter]")
{
    Limiter limiter;
    const auto input = Sine(440.0f, 1000, 1.0f);
    auto output = input;
    limiter.process(output.data(), 1000);
    REQUIRE(output == input);
}

TEST_CASE("the limiter never goes over the ceiling and keeps the shape of the sound", "[audio][effects][limiter]")
{
    Limiter limiter;
    auto pcm = Sine(440.0f, second, 3.0f);
    limiter.process(pcm.data(), second);
    REQUIRE(Peak(pcm) <= 1.0f);

    // A clamp would hold 1.0 for most of each period
    std::size_t flat = 0;
    for (std::size_t i = second / 2; i < second; ++i) flat += std::abs(pcm[i * 2]) >= 0.999f;
    REQUIRE(flat < second / 2 / 10);
    REQUIRE(Peak(pcm, second / 2) > 0.95f);
}

TEST_CASE("the limiter keeps the balance and comes back after a loud sound", "[audio][effects][limiter]")
{
    Limiter limiter;
    std::vector<float> loud{ 4.0f, -2.0f };
    limiter.process(loud.data(), 1);
    REQUIRE_THAT(loud[0], WithinAbs(1.0f, 1e-6f));
    REQUIRE_THAT(loud[1], WithinAbs(-0.5f, 1e-6f));

    auto quiet = Constant(0.5f, second);
    limiter.process(quiet.data(), second);
    REQUIRE(quiet[0] < 0.2f);
    REQUIRE(std::ranges::is_sorted(quiet));
    REQUIRE(quiet.back() == 0.5f);
}

TEST_CASE("the slowest release still comes all the way back", "[audio][effects][limiter]")
{
    Limiter limiter;
    limiter.setParameters(std::array{ 1.0f, 5.0f });
    Ducker ducker;
    ducker.setParameters(std::array{ 1.0f, 0.1f, 0.001f, 5.0f });

    std::vector<float> loud{ 4.0f, 4.0f };
    limiter.process(loud.data(), 1);
    float gain = 1.0f;
    for (std::size_t i = 0; i < second; ++i) gain = ducker.next(1.0f, 1.0f);
    REQUIRE(gain == 0.0f);

    for (int i = 0; i < 60; ++i) {
        auto quiet = Constant(0.1f, second);
        limiter.process(quiet.data(), second);
        loud = quiet;
        for (std::size_t j = 0; j < second; ++j) gain = ducker.next(0.0f, 0.0f);
    }
    REQUIRE(loud.back() == 0.1f);
    REQUIRE(gain == 1.0f);
}

TEST_CASE("ducking turns down while the trigger is heard, then back up", "[audio][effects][ducking]")
{
    Ducker ducker;
    ducker.setParameters(std::array{ 0.75f, 0.1f, 0.01f, 0.1f });
    REQUIRE(ducker.next(0.0f, 0.0f) == 1.0f);
    REQUIRE(ducker.next(0.05f, -0.05f) == 1.0f);

    const auto trigger = Sine(50.0f);
    float gain = 1.0f;
    for (std::size_t i = 0; i < second; ++i) {
        const float next = ducker.next(trigger[i * 2], trigger[i * 2 + 1]);
        if (i > 1000)
            REQUIRE(next <= gain);
        gain = next;
    }
    REQUIRE(gain == 0.25f);

    for (std::size_t i = 0; i < second * 2; ++i) gain = ducker.next(0.0f, 0.0f);
    REQUIRE(gain == 1.0f);
}

TEST_CASE("an effect only changes its own bus", "[audio][effects][server]")
{
    ScopedBuses reset;
    const PlayingVoice music(Sine(8000.0f), Bus::Music);
    Server().busSetEffect(Bus::Music, EffectKind::LowPass);
    REQUIRE(Peak(Mix(second), second / 2) < 0.02f);

    const PlayingVoice sfx(Sine(8000.0f), Bus::Sfx);
    REQUIRE_THAT(Amplitude(Mix(second), second / 2), WithinAbs(0.5f, 0.01f));
}

TEST_CASE("an effect on Master changes every bus", "[audio][effects][server]")
{
    const Bus bus = GENERATE(Bus::Master, Bus::Music, Bus::Sfx);
    ScopedBuses reset;
    const PlayingVoice voice(Sine(8000.0f), bus);
    Server().busSetEffect(Bus::Master, EffectKind::LowPass);
    REQUIRE(Peak(Mix(second), second / 2) < 0.02f);
}

TEST_CASE("effects of a bus add up and removing them restores the sound", "[audio][effects][server]")
{
    ScopedBuses reset;
    const PlayingVoice voice(Sine(1000.0f, second * 3), Bus::Sfx);
    Server().busSetEffect(Bus::Sfx, EffectKind::LowPass, std::array{ EffectValue{ "cutoff", 1000.0f } });
    Server().busSetEffect(Bus::Sfx, EffectKind::HighPass, std::array{ EffectValue{ "cutoff", 1000.0f } });
    REQUIRE_THAT(Peak(Mix(second), second / 2), WithinAbs(0.25f, 0.01f));

    Server().busRemoveEffect(Bus::Sfx, EffectKind::LowPass);
    Server().busRemoveEffect(Bus::Sfx, EffectKind::HighPass);
    Server().busRemoveEffect(Bus::Sfx, EffectKind::HighPass);
    REQUIRE_THAT(Peak(Mix(second)), WithinAbs(0.5f, 1e-4f));
}

TEST_CASE("the tail of an effect outlives its voice", "[audio][effects][server]")
{
    ScopedBuses reset;
    Server().busSetEffect(Bus::Sfx, EffectKind::Delay, std::array{ EffectValue{ "time", 0.5f }, EffectValue{ "mix", 0.5f } });
    {
        const PlayingVoice voice(Constant(0.5f, 100), Bus::Sfx);
        Mix(200);
    }
    REQUIRE_THAT(Peak(Mix(second)), WithinAbs(0.25f, 1e-4f));

    // Muting the bus mutes the tail too
    Server().busSetVolume(Bus::Sfx, 0.0f);
    REQUIRE(Peak(Mix(second)) == 0.0f);
}

TEST_CASE("changing a setting keeps the others and the memory of the effect", "[audio][effects][server]")
{
    ScopedBuses reset;
    Server().busSetEffect(Bus::Sfx, EffectKind::Delay, std::array{ EffectValue{ "time", 0.1f }, EffectValue{ "feedback", 0.0f } });
    const PlayingVoice voice(Constant(0.5f, 10), Bus::Sfx);
    Mix(100);

    Server().busSetEffect(Bus::Sfx, EffectKind::Delay, std::array{ EffectValue{ "mix", 1.0f } });
    REQUIRE_THAT(Peak(Mix(second)), WithinAbs(0.5f, 1e-4f));

    const auto settings = Server().busGetEffect(Bus::Sfx, EffectKind::Delay);
    REQUIRE(settings.has_value());
    REQUIRE(settings->size() == 3);
    REQUIRE(settings->at(0).name == "time");
    REQUIRE(settings->at(0).value == 0.1f);
    REQUIRE(settings->at(2).value == 1.0f);
}

TEST_CASE("settings are kept in range and bad ones are ignored", "[audio][effects][server]")
{
    ScopedBuses reset;
    REQUIRE_FALSE(Server().busGetEffect(Bus::Music, EffectKind::LowPass).has_value());

    const auto cutoff = [] { return Server().busGetEffect(Bus::Music, EffectKind::LowPass)->at(0).value; };
    const auto set = [](std::string_view name, float value) {
        Server().busSetEffect(Bus::Music, EffectKind::LowPass, std::array{ EffectValue{ name, value } });
    };
    set("cutoff", 1e9f);
    REQUIRE(cutoff() == 20000.0f);
    set("cutoff", -5.0f);
    REQUIRE(cutoff() == 20.0f);
    set("cutoff", std::numeric_limits<float>::quiet_NaN());
    set("cutoff", std::numeric_limits<float>::infinity());
    set("wet", 1.0f);
    REQUIRE(cutoff() == 20.0f);

    Server().busSetEffect(static_cast<Bus>(200), EffectKind::LowPass);
    Server().busSetEffect(Bus::Sfx, static_cast<EffectKind>(200));
    Server().busRemoveEffect(static_cast<Bus>(200), EffectKind::LowPass);
    Server().busRemoveEffect(Bus::Sfx, static_cast<EffectKind>(200));
    REQUIRE_FALSE(Server().busGetEffect(Bus::Sfx, static_cast<EffectKind>(200)).has_value());
    Mix(16);
}

TEST_CASE("a sample with bad values does not break a bus for good", "[audio][effects][server]")
{
    const EffectKind kind = AnyEffect();
    const float bad = GENERATE(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());
    const Bus bus = GENERATE(Bus::Master, Bus::Music);

    ScopedBuses reset;
    Server().busSetEffect(bus, kind);
    {
        const PlayingVoice voice(Constant(bad, 16), bus);
        for (const float value : Mix(64)) REQUIRE(std::isfinite(value));
    }
    for (const float value : Mix(second)) {
        if (!(std::abs(value) <= 1.0f))
            FAIL("value " << value);
    }

    // Once the effect is gone, nothing is left of it in the limiter
    Server().busRemoveEffect(bus, kind);
    Mix(second * 2);
    const PlayingVoice voice(Constant(0.5f, 16), bus);
    REQUIRE(Peak(Mix(16)) == 0.5f);
}

TEST_CASE("ducking lowers the music while an sfx plays", "[audio][effects][ducking][server]")
{
    ScopedBuses reset;
    const PlayingVoice music(Constant(0.4f, second * 10), Bus::Music);
    Server().busSetDucking(Bus::Music, Bus::Sfx, std::array{ EffectValue{ "amount", 0.75f }, EffectValue{ "release", 0.1f } });
    REQUIRE(Peak(Mix(100)) == 0.4f);

    {
        const PlayingVoice sfx(Constant(0.2f, second), Bus::Sfx);
        REQUIRE_THAT(Mix(second / 2).back(), WithinAbs(0.2f + 0.1f, 1e-3f));
        // A trigger that is not heard does not duck
        Server().busSetVolume(Bus::Sfx, 0.0f);
        REQUIRE_THAT(Mix(second * 2).back(), WithinAbs(0.4f, 1e-3f));
    }

    Server().busSetVolume(Bus::Sfx, 1.0f);
    Server().busRemoveDucking(Bus::Music);
    const PlayingVoice sfx(Constant(0.2f, second), Bus::Sfx);
    REQUIRE_THAT(Mix(second / 2).back(), WithinAbs(0.6f, 1e-3f));
}

TEST_CASE("two buses can duck each other", "[audio][effects][ducking][server]")
{
    ScopedBuses reset;
    const PlayingVoice music(Constant(0.4f, second * 3), Bus::Music);
    const PlayingVoice sfx(Constant(0.2f, second * 3), Bus::Sfx);
    Server().busSetDucking(Bus::Music, Bus::Sfx, std::array{ EffectValue{ "amount", 0.5f } });
    Server().busSetDucking(Bus::Sfx, Bus::Music, std::array{ EffectValue{ "amount", 0.5f } });
    REQUIRE_THAT(Mix(second).back(), WithinAbs(0.3f, 1e-3f));
}

TEST_CASE("ducking refuses Master and the bus itself", "[audio][effects][ducking][server]")
{
    ScopedBuses reset;
    Server().busSetDucking(Bus::Master, Bus::Sfx, std::array{ EffectValue{ "amount", 1.0f } });
    Server().busSetDucking(Bus::Sfx, Bus::Master, std::array{ EffectValue{ "amount", 1.0f } });
    Server().busSetDucking(Bus::Sfx, Bus::Sfx, std::array{ EffectValue{ "amount", 1.0f } });
    Server().busSetDucking(static_cast<Bus>(200), Bus::Sfx);
    Server().busSetDucking(Bus::Sfx, static_cast<Bus>(200));
    Server().busRemoveDucking(static_cast<Bus>(200));

    const PlayingVoice sfx(Constant(0.5f, second), Bus::Sfx);
    REQUIRE(Mix(second / 2).back() == 0.5f);
}
