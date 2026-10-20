#include <chrono>
#include <cmath>
#include <cstddef>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "offscreen_video.hpp"
#include "server/rendering/rendering_server.hpp"

using namespace atmo::server::rendering;

namespace
{
    constexpr int sprite_count = 10000;
    constexpr int texture_count = 50;
    constexpr int warmup_frames = 10;
    constexpr int measured_frames = 120;

    void RunBenchmark(PlatformKind platform)
    {
        RenderingServer &server = RenderingServer::Instance();
        REQUIRE(server.init({ .platform = platform }));
        if (server.platformKind() != platform) {
            server.shutdown();
            SKIP("Rendering platform unavailable");
        }

        CanvasRid canvas = server.canvasCreate();
        ViewportRid viewport = server.viewportCreate();
        server.viewportSetSize(viewport, { 1920, 1080 });
        server.viewportSetCanvas(viewport, canvas);

        std::vector<TextureRid> textures;
        for (int i = 0; i < texture_count; ++i) {
            std::vector<std::byte> pixels(32 * 32 * 4, std::byte{ static_cast<unsigned char>(i * 5) });
            textures.push_back(server.texture2dCreate({ .size = { 32, 32 } }, pixels));
        }

        std::vector<CanvasItemRid> items;
        items.reserve(sprite_count);
        for (int i = 0; i < sprite_count; ++i) {
            CanvasItemRid item = server.canvasItemCreate(canvas);
            server.canvasItemAddTextureRect(item, Rect2::Centered({ 32.0f, 32.0f }), textures[static_cast<std::size_t>(i % texture_count)]);
            items.push_back(item);
        }

        double main_ms = 0.0;
        double render_ms = 0.0;
        FrameStats stats;
        for (int frame = 0; frame < warmup_frames + measured_frames; ++frame) {
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < sprite_count; ++i) {
                const float t = static_cast<float>(frame) * 0.02f + static_cast<float>(i);
                server.canvasItemSetTransform(
                    items[static_cast<std::size_t>(i)],
                    Affine2d::FromTrs({ 960.0f + 900.0f * std::sin(t * 0.37f), 540.0f + 500.0f * std::cos(t * 0.53f) }, t * 10.0f, { 1.0f, 1.0f }));
            }
            server.sync();
            const double elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

            stats = server.lastFrameStats();
            if (frame >= warmup_frames) {
                main_ms += elapsed;
                render_ms += stats.render_cpu_ms;
            }
        }
        server.waitIdle();
        stats = server.lastFrameStats();

        WARN(
            PlatformKindName(platform) << ": " << stats.instances << " instances in " << stats.draw_calls << " draw calls, render thread "
                                       << render_ms / measured_frames << " ms/frame, main thread (10k setters + sync) " << main_ms / measured_frames
                                       << " ms/frame");

        CHECK(stats.instances == static_cast<std::uint32_t>(sprite_count));
        CHECK(stats.draw_calls < 5);
        server.shutdown();
    }
} // namespace

TEST_CASE("10k sprites from 50 textures on the Null platform", "[.benchmark]")
{
    RunBenchmark(PlatformKind::Null);
}

TEST_CASE("10k sprites from 50 textures on Vulkan", "[.benchmark]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable");
    RunBenchmark(PlatformKind::Vulkan);
}
