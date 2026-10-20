#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "server/rendering/api/null/null_platform.hpp"
#include "server/rendering/platform/handle_pool.hpp"
#include "server/rendering/renderer/test_pattern.hpp"

using namespace atmo::server::rendering;

TEST_CASE("handle pools reject stale handles", "[rendering][platform]")
{
    HandlePool<TextureHandle, int> pool;

    TextureHandle first = pool.insert(1);
    REQUIRE(first.isValid());
    REQUIRE(*pool.get(first) == 1);

    REQUIRE(pool.remove(first) == 1);
    REQUIRE(pool.get(first) == nullptr);
    REQUIRE_FALSE(pool.remove(first).has_value());

    TextureHandle second = pool.insert(2);
    REQUIRE((second.value & HandlePool<TextureHandle, int>::slot_mask) == (first.value & HandlePool<TextureHandle, int>::slot_mask));
    REQUIRE(second != first);
    REQUIRE(pool.get(first) == nullptr);
    REQUIRE(*pool.get(second) == 2);
    REQUIRE_THROWS_AS(pool.at(TextureHandle{}), std::invalid_argument);
}

TEST_CASE("the Null platform enforces the frame protocol", "[rendering][platform]")
{
    NullPlatform platform;
    REQUIRE(platform.initialize({}));

    SwapchainHandle swapchain = platform.swapchainCreate({}, { .size = { 16, 16 } });

    REQUIRE_THROWS_AS(platform.swapchainAcquire(swapchain), std::logic_error);
    REQUIRE_THROWS_AS(platform.commandListAcquire(QueueType::Graphics), std::logic_error);
    REQUIRE_THROWS_AS(platform.frameEnd(), std::logic_error);

    platform.frameBegin();
    REQUIRE_THROWS_AS(platform.frameBegin(), std::logic_error);

    TextureHandle image = platform.swapchainAcquire(swapchain);
    REQUIRE(image.isValid());
    REQUIRE(platform.textureSize(image).x == 16);
    REQUIRE_THROWS_AS(platform.swapchainAcquire(swapchain), std::logic_error);
    REQUIRE_THROWS_AS(platform.swapchainPresent(swapchain), std::logic_error);

    CommandList &list = platform.commandListAcquire(QueueType::Graphics);
    REQUIRE_THROWS_AS(list.draw(3, 1, 0, 0), std::logic_error);

    const ColorAttachment attachment{ .texture = image };
    list.renderPassBegin({ .color_attachments = { &attachment, 1 } });
    REQUIRE_THROWS_AS(list.barrier({}), std::logic_error);
    REQUIRE_THROWS_AS(list.draw(3, 1, 0, 0), std::logic_error);
    CommandList *lists[] = { &list };
    REQUIRE_THROWS_AS(platform.submit(lists), std::logic_error);
    list.renderPassEnd();

    platform.submit(lists);
    platform.swapchainPresent(swapchain);
    platform.frameEnd();

    REQUIRE(platform.frameCount() == 1);
    REQUIRE(platform.presentCount() == 1);
    platform.shutdown();
}

TEST_CASE("minimized swapchains are skipped", "[rendering][platform]")
{
    NullPlatform platform;
    REQUIRE(platform.initialize({}));

    SwapchainHandle swapchain = platform.swapchainCreate({}, { .size = { 16, 16 } });
    platform.swapchainResize(swapchain, { 0, 0 });

    platform.frameBegin();
    REQUIRE_FALSE(platform.swapchainAcquire(swapchain).isValid());
    platform.frameEnd();

    REQUIRE(platform.presentCount() == 0);
}

TEST_CASE("Null textures keep uploaded pixels for readback", "[rendering][platform]")
{
    NullPlatform platform;
    REQUIRE(platform.initialize({}));

    TextureHandle texture = platform.textureCreate({ .size = { 2, 2 }, .usage = TextureUsage::Sampled | TextureUsage::CopyDst | TextureUsage::CopySrc });

    const std::array<std::byte, 4> red{ std::byte{ 255 }, std::byte{ 0 }, std::byte{ 0 }, std::byte{ 255 } };
    platform.textureUpload(texture, { .x = 1, .y = 1, .width = 1, .height = 1 }, red);

    std::vector<std::byte> pixels = platform.textureReadback(texture);
    REQUIRE(pixels.size() == 16);
    REQUIRE(pixels[12] == std::byte{ 255 });
    REQUIRE(pixels[0] == std::byte{ 0 });

    REQUIRE_THROWS_AS(platform.textureUpload(texture, { .x = 2, .y = 0, .width = 1, .height = 1 }, red), std::out_of_range);
    REQUIRE_THROWS_AS(platform.textureUpload(texture, { .width = 2, .height = 2 }, red), std::invalid_argument);

    platform.destroy(texture);
    REQUIRE_THROWS_AS(platform.destroy(texture), std::invalid_argument);
}

TEST_CASE("the test pattern releases everything it creates", "[rendering][platform]")
{
    NullPlatform platform;
    REQUIRE(platform.initialize({}));
    SwapchainHandle swapchain = platform.swapchainCreate({}, { .size = { 16, 16 } });

    {
        TestPattern pattern(platform);
        platform.frameBegin();
        pattern.render({ &swapchain, 1 });
        platform.frameEnd();

        REQUIRE(pattern.isAvailable());
        REQUIRE(platform.drawCount() == 1);
        REQUIRE(platform.liveObjectCount() > 0);
    }

    REQUIRE(platform.liveObjectCount() == 0);
}

TEST_CASE("platform names round-trip", "[rendering][platform]")
{
    for (PlatformKind kind : { PlatformKind::Null, PlatformKind::Vulkan, PlatformKind::Metal, PlatformKind::D3D12 })
        REQUIRE(ParsePlatformKind(PlatformKindName(kind)) == kind);
    REQUIRE_FALSE(ParsePlatformKind("opengl").has_value());
}

TEST_CASE("automatic selection follows the native API of each OS", "[rendering][platform]")
{
    std::span<const PlatformKind> order = AutomaticPlatformOrder();
    REQUIRE(order.size() >= 2);
    REQUIRE(order.back() == PlatformKind::Null);
    REQUIRE(std::ranges::find(order, PlatformKind::Vulkan) != order.end());
#if defined(__APPLE__)
    REQUIRE(order.front() == PlatformKind::Metal);
    REQUIRE(std::ranges::find(order, PlatformKind::D3D12) == order.end());
#elif defined(_WIN32)
    REQUIRE(order.front() == PlatformKind::D3D12);
#else
    REQUIRE(order.front() == PlatformKind::Vulkan);
#endif
}

TEST_CASE("an unavailable platform falls back to automatic selection", "[rendering][platform]")
{
    auto automatic = CreatePlatform({});
    REQUIRE(automatic);
    automatic->shutdown();

    auto fallback = CreatePlatform({ .platform = PlatformKind::D3D12 });
    REQUIRE(fallback);
    REQUIRE(fallback->kind() != PlatformKind::D3D12);
    fallback->shutdown();

    auto explicit_null = CreatePlatform({ .platform = PlatformKind::Null });
    REQUIRE(explicit_null);
    REQUIRE(explicit_null->kind() == PlatformKind::Null);
    explicit_null->shutdown();
}
