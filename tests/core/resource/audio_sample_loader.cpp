#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "core/resource/audio_sample.hpp"
#include "core/resource/loaders/audio_sample_loader.hpp"
#include "core/resource/resource_manager.hpp"
#include "server/audio/audio_server.hpp"

using atmo::core::resource::AudioSample;
using atmo::core::resource::AudioSampleLoader;
using atmo::core::resource::ResourceManager;
using atmo::server::audio::AudioServer;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr std::uint16_t format_pcm = 1;
    constexpr std::uint16_t format_float = 3;

    struct TempDirectory {
        std::filesystem::path root = std::filesystem::temp_directory_path() / "atmo-audio-loader-test";

        TempDirectory()
        {
            std::filesystem::remove_all(root);
            std::filesystem::create_directories(root);
        }
        ~TempDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(root, error);
        }

        std::string write(const std::string &name, const std::string &content) const
        {
            std::ofstream out(root / name, std::ios::binary | std::ios::trunc);
            out << content;
            return (root / name).generic_string();
        }
    };

    template <typename T> void Append(std::string &out, T value)
    {
        // WAV files are little endian
        for (std::size_t i = 0; i < sizeof(T); ++i) out.push_back(static_cast<char>((static_cast<std::uint64_t>(value) >> (8 * i)) & 0xFF));
    }

    std::string Wav(std::uint16_t format, std::uint16_t channels, std::uint32_t rate, std::uint16_t bits, const std::string &data)
    {
        const auto frame_size = static_cast<std::uint16_t>(channels * bits / 8);
        std::string out = "RIFF";
        Append<std::uint32_t>(out, static_cast<std::uint32_t>(36 + data.size()));
        out += "WAVEfmt ";
        Append<std::uint32_t>(out, 16);
        Append<std::uint16_t>(out, format);
        Append<std::uint16_t>(out, channels);
        Append<std::uint32_t>(out, rate);
        Append<std::uint32_t>(out, rate * frame_size);
        Append<std::uint16_t>(out, frame_size);
        Append<std::uint16_t>(out, bits);
        out += "data";
        Append<std::uint32_t>(out, static_cast<std::uint32_t>(data.size()));
        return out + data;
    }

    std::string Pcm16(const std::vector<float> &values)
    {
        std::string data;
        for (float value : values) Append<std::uint16_t>(data, static_cast<std::uint16_t>(static_cast<std::int16_t>(value * 32767.0f)));
        return data;
    }

    std::vector<float> Sine(int rate, int channels, float seconds)
    {
        std::vector<float> values;
        for (int i = 0; i < static_cast<int>(static_cast<float>(rate) * seconds); ++i)
            for (int c = 0; c < channels; ++c)
                values.push_back(0.5f * std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * static_cast<float>(i) / static_cast<float>(rate)));
        return values;
    }

    std::vector<float> Render(const AudioSample &sample, int frames)
    {
        auto &server = AudioServer::Instance();
        const auto voice = server.voiceCreate();
        server.voiceSetSample(voice, sample.rid());
        server.voicePlay(voice);
        std::vector<float> out(static_cast<std::size_t>(frames) * AudioServer::channels);
        server.mix(out.data(), frames);
        server.voiceFree(voice);
        return out;
    }
} // namespace

TEST_CASE("a WAV already in the server format is loaded untouched", "[audio][resource]")
{
    TempDirectory directory;
    AudioSampleLoader loader;
    const auto sample = loader.load(directory.write("a.wav", Wav(format_pcm, 2, 48000, 16, Pcm16({ 0.5f, -0.5f, 0.25f, 0.0f }))));

    REQUIRE(AudioServer::Instance().sampleGetFrames(sample->rid()) == 2);
    const std::vector<float> out = Render(*sample, 2);
    const std::vector<float> expected{ 0.5f, -0.5f, 0.25f, 0.0f };
    for (std::size_t i = 0; i < expected.size(); ++i) REQUIRE_THAT(out[i], WithinAbs(expected[i], 1e-3));
}

TEST_CASE("mono is sent to both sides", "[audio][resource]")
{
    TempDirectory directory;
    AudioSampleLoader loader;
    const auto sample = loader.load(directory.write("mono.wav", Wav(format_pcm, 1, 48000, 16, Pcm16({ 0.5f, -0.25f }))));

    REQUIRE(AudioServer::Instance().sampleGetFrames(sample->rid()) == 2);
    const std::vector<float> out = Render(*sample, 2);
    const std::vector<float> expected{ 0.5f, 0.5f, -0.25f, -0.25f };
    for (std::size_t i = 0; i < expected.size(); ++i) REQUIRE_THAT(out[i], WithinAbs(expected[i], 1e-3));
}

TEST_CASE("every rate and channel count keeps its duration and pitch", "[audio][resource]")
{
    const int rate = GENERATE(8000, 22050, 44100, 48000, 96000);
    const int channels = GENERATE(1, 2);
    INFO(rate << " Hz, " << channels << " channels");

    TempDirectory directory;
    AudioSampleLoader loader;
    const auto sample = loader.load(directory.write(
        "sine.wav", Wav(format_pcm, static_cast<std::uint16_t>(channels), static_cast<std::uint32_t>(rate), 16, Pcm16(Sine(rate, channels, 0.5f)))));
    REQUIRE_THAT(sample->duration(), WithinAbs(0.5, 0.001));

    const std::vector<float> out = Render(*sample, 24000);
    int cycles = 0;
    for (std::size_t i = 2; i < out.size(); i += 2) cycles += out[i - 2] < 0.0f && out[i] >= 0.0f;
    REQUIRE(cycles >= 219);
    REQUIRE(cycles <= 220);
}

TEST_CASE("8-bit and float WAV files are converted", "[audio][resource]")
{
    TempDirectory directory;
    AudioSampleLoader loader;

    const auto u8 = loader.load(directory.write("u8.wav", Wav(format_pcm, 2, 48000, 8, std::string("\xC0\x40", 2))));
    const std::vector<float> u8_out = Render(*u8, 1);
    REQUIRE_THAT(u8_out[0], WithinAbs(0.5, 0.01));
    REQUIRE_THAT(u8_out[1], WithinAbs(-0.5, 0.01));

    std::string floats;
    Append<std::uint32_t>(floats, 0x3F000000); // 0.5f
    Append<std::uint32_t>(floats, 0xBE800000); // -0.25f
    const auto f32 = loader.load(directory.write("f32.wav", Wav(format_float, 2, 48000, 32, floats)));
    const std::vector<float> f32_out = Render(*f32, 1);
    REQUIRE_THAT(f32_out[0], WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(f32_out[1], WithinAbs(-0.25, 1e-6));
}

TEST_CASE("invalid audio files throw LoadException", "[audio][resource]")
{
    TempDirectory directory;
    AudioSampleLoader loader;
    const std::string valid = Wav(format_pcm, 2, 48000, 16, Pcm16({ 0.5f, 0.5f }));

    REQUIRE_THROWS_AS(loader.load(directory.write("empty.wav", "")), AudioSampleLoader::LoadException);
    REQUIRE_THROWS_AS(loader.load(directory.write("text.wav", "this is not a sound")), AudioSampleLoader::LoadException);
    REQUIRE_THROWS_AS(loader.load(directory.write("header.wav", valid.substr(0, 20))), AudioSampleLoader::LoadException);
    REQUIRE_THROWS_AS(loader.load(directory.write("silent.wav", Wav(format_pcm, 2, 48000, 16, ""))), AudioSampleLoader::LoadException);
}

TEST_CASE("a chunk larger than the file never escapes as another exception", "[audio][resource]")
{
    TempDirectory directory;
    AudioSampleLoader loader;
    std::string broken = "RIFF";
    Append<std::uint32_t>(broken, 4096);
    broken += "WAVEjunk";
    Append<std::uint32_t>(broken, 2048);
    broken += "short";

    REQUIRE_THROWS_AS(loader.load(directory.write("chunk.wav", broken)), AudioSampleLoader::LoadException);
}

TEST_CASE("a missing audio file throws", "[audio][resource]")
{
    TempDirectory directory;
    AudioSampleLoader loader;
    REQUIRE_THROWS_AS(loader.load((directory.root / "missing.wav").generic_string()), std::runtime_error);
}

TEST_CASE("the ResourceManager shares audio and releases it with its last owner", "[audio][resource]")
{
    TempDirectory directory;
    auto &server = AudioServer::Instance();
    const std::string path = directory.write("shared.wav", Wav(format_pcm, 2, 48000, 16, Pcm16({ 0.5f, 0.5f })));

    auto a = ResourceManager::GetInstance().getResource<AudioSample>(path);
    auto b = ResourceManager::GetInstance().getResource<AudioSample>(path);
    REQUIRE(a != nullptr);
    REQUIRE(a.get() == b.get());

    const auto rid = a->rid();
    a.reset();
    REQUIRE(server.sampleGetFrames(rid) == 1);
    b.reset();
    REQUIRE(server.sampleGetFrames(rid) == 0);

    auto reloaded = ResourceManager::GetInstance().getResource<AudioSample>(path);
    REQUIRE(server.sampleGetFrames(reloaded->rid()) == 1);
}

TEST_CASE("audio loads without an audio device", "[audio][resource]")
{
    REQUIRE_FALSE(AudioServer::Instance().isInitialized());

    TempDirectory directory;
    AudioSampleLoader loader;
    const auto sample = loader.load(directory.write("headless.wav", Wav(format_pcm, 1, 44100, 16, Pcm16({ 0.1f, 0.2f }))));
    REQUIRE(sample->rid().isValid());
}
