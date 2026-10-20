#if defined(ATMO_RENDERING_VULKAN)

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "SDL3/SDL.h"
#include "offscreen_video.hpp"
#include "server/rendering/api/vulkan/vulkan_platform.hpp"
#include "server/rendering/renderer/test_pattern.hpp"
#include "server/rendering/rendering_server.hpp"
#include "server/rendering/shader/builtin_shaders.hpp"

using namespace atmo::server::rendering;

namespace
{
    struct VulkanFixture {
        VulkanFixture()
        {
            if (video.available)
                ready = platform.initialize({ .gpu_validation = true });
        }

        ~VulkanFixture()
        {
            platform.shutdown();
        }

        OffscreenVideo video;
        VulkanPlatform platform;
        bool ready = false;
    };

    struct alignas(16) Constants {
        std::array<float, 4> rect;
        std::array<float, 4> color;
        std::uint32_t texture;
        std::uint32_t sampler;
        std::array<std::uint32_t, 2> padding;
    };

    std::array<std::uint8_t, 4> Pixel(const std::vector<std::byte> &pixels, int width, int x, int y)
    {
        const std::size_t offset = ((static_cast<std::size_t>(y) * width) + x) * 4;
        return { static_cast<std::uint8_t>(pixels[offset]),
                 static_cast<std::uint8_t>(pixels[offset + 1]),
                 static_cast<std::uint8_t>(pixels[offset + 2]),
                 static_cast<std::uint8_t>(pixels[offset + 3]) };
    }
} // namespace

#define REQUIRE_VULKAN(fixture)                                                 \
    do {                                                                        \
        if (!(fixture).video.available)                                         \
            SKIP("SDL offscreen video driver unavailable: " << SDL_GetError()); \
        if (!(fixture).ready)                                                   \
            SKIP("No Vulkan 1.3 device with the required features");            \
    } while (false)

TEST_CASE("the Vulkan platform draws a textured quad offscreen", "[rendering][vulkan]")
{
    VulkanFixture vulkan;
    REQUIRE_VULKAN(vulkan);
    VulkanPlatform &platform = vulkan.platform;

    constexpr int size = 64;
    std::optional<ShaderBlob> blob = BuiltinShaderBlob(BuiltinShader::TestPattern, ShaderTarget::SpirV);
    REQUIRE(blob.has_value());

    ShaderModuleHandle module = platform.shaderModuleCreate(*blob);
    PipelineHandle pipeline = platform.graphicsPipelineCreate(
        {
            .vertex = { .module = module, .entry_point = "vertexMain" },
            .fragment = { .module = module, .entry_point = "fragmentMain" },
            .color_format = TextureFormat::Rgba8Unorm,
            .blend = BlendMode::Opaque,
        });
    TextureHandle target = platform.textureCreate({ .size = { size, size }, .usage = TextureUsage::RenderTarget | TextureUsage::CopySrc });
    TextureHandle texture = platform.textureCreate({ .size = { 1, 1 } });
    SamplerHandle sampler = platform.samplerCreate({ .filter = Filter::Nearest });

    const auto texel = TestPattern::light_texel;
    const std::array<std::byte, 4> texel_bytes{ std::byte{ texel[0] }, std::byte{ texel[1] }, std::byte{ texel[2] }, std::byte{ texel[3] } };
    platform.textureUpload(texture, { .width = 1, .height = 1 }, texel_bytes);

    const std::array<std::uint16_t, 6> indices{ 0, 1, 2, 2, 1, 3 };
    BufferHandle index_buffer = platform.bufferCreate({ .size = sizeof(indices), .usage = BufferUsage::Index, .memory = MemoryType::Upload });
    platform.bufferWrite(index_buffer, 0, std::as_bytes(std::span(indices)));

    platform.frameBegin();
    CommandList &list = platform.commandListAcquire(QueueType::Graphics);

    const TextureBarrier to_target{ .texture = target, .after = ResourceState::RenderTarget };
    list.barrier({ &to_target, 1 });
    const ColorAttachment attachment{ .texture = target, .clear_color = { 0.0f, 0.0f, 1.0f, 1.0f } };
    list.renderPassBegin({ .color_attachments = { &attachment, 1 } });
    list.viewportSet({ .width = size, .height = size });
    list.scissorSet({ .width = size, .height = size });
    list.pipelineBind(pipeline);

    const Constants constants{
        .rect = { -1.0f, 0.0f, 2.0f, 1.0f },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texture = platform.bindlessIndex(texture),
        .sampler = platform.bindlessIndex(sampler),
        .padding = {},
    };
    list.constantsPush(std::as_bytes(std::span(&constants, 1)));
    list.indexBufferBind(index_buffer, IndexType::Uint16, 0);
    list.drawIndexed(6, 1, 0, 0, 0);
    list.renderPassEnd();

    CommandList *lists[] = { &list };
    platform.submit(lists);
    platform.frameEnd();

    std::vector<std::byte> pixels = platform.textureReadback(target);
    REQUIRE(pixels.size() == static_cast<std::size_t>(size * size * 4));

    const std::array<std::uint8_t, 4> top = Pixel(pixels, size, size / 2, size / 4);
    const std::array<std::uint8_t, 4> bottom = Pixel(pixels, size, size / 2, (3 * size) / 4);
    CHECK(top == texel);
    CHECK(bottom == std::array<std::uint8_t, 4>{ 0, 0, 255, 255 });

    platform.destroy(index_buffer);
    platform.destroy(sampler);
    platform.destroy(texture);
    platform.destroy(target);
    platform.destroy(pipeline);
    platform.destroy(module);

    if (!platform.validationEnabled())
        WARN("Vulkan validation layer not installed, validation was not checked");
    REQUIRE(platform.validationErrorCount() == 0);
}

TEST_CASE("Vulkan uploads recorded between frames reach the GPU", "[rendering][vulkan]")
{
    VulkanFixture vulkan;
    REQUIRE_VULKAN(vulkan);
    VulkanPlatform &platform = vulkan.platform;

    TextureHandle texture = platform.textureCreate({ .size = { 4, 4 }, .usage = TextureUsage::Sampled | TextureUsage::CopyDst | TextureUsage::CopySrc });

    std::vector<std::byte> pixels(4 * 4 * 4);
    for (std::size_t i = 0; i < pixels.size(); ++i) pixels[i] = static_cast<std::byte>(i);
    platform.textureUpload(texture, { .width = 4, .height = 4 }, pixels);

    REQUIRE(platform.textureReadback(texture) == pixels);

    platform.frameBegin();
    std::vector<std::byte> row(4 * 4, std::byte{ 0xAB });
    platform.textureUpload(texture, { .y = 2, .width = 4, .height = 1 }, row);
    platform.frameEnd();

    std::vector<std::byte> expected = pixels;
    std::fill(expected.begin() + (2 * 16), expected.begin() + (3 * 16), std::byte{ 0xAB });
    REQUIRE(platform.textureReadback(texture) == expected);

    platform.destroy(texture);
    REQUIRE(platform.validationErrorCount() == 0);
}

TEST_CASE("Vulkan swapchains present the test pattern and survive a resize", "[rendering][vulkan]")
{
    VulkanFixture vulkan;
    REQUIRE_VULKAN(vulkan);
    VulkanPlatform &platform = vulkan.platform;
    if (!platform.caps().presentation)
        SKIP("This Vulkan instance cannot present to windows");

    SDL_Window *window = SDL_CreateWindow("vulkan", 64, 64, platform.windowFlags());
    REQUIRE(window != nullptr);

    SwapchainHandle swapchain = platform.swapchainCreate({ .sdl_window = window }, { .size = { 64, 64 } });
    if (!swapchain.isValid()) {
        SDL_DestroyWindow(window);
        SKIP("No Vulkan surface for offscreen windows");
    }

    {
        TestPattern pattern(platform);
        for (int frame = 0; frame < 3; ++frame) {
            platform.frameBegin();
            pattern.render({ &swapchain, 1 });
            platform.frameEnd();
        }
        REQUIRE(pattern.isAvailable());

        SDL_SetWindowSize(window, 128, 96);
        platform.swapchainResize(swapchain, { 128, 96 });
        for (int frame = 0; frame < 3; ++frame) {
            platform.frameBegin();
            pattern.render({ &swapchain, 1 });
            platform.frameEnd();
        }
    }

    platform.swapchainDestroy(swapchain);
    SDL_DestroyWindow(window);
    REQUIRE(platform.validationErrorCount() == 0);
}

TEST_CASE("the RenderingServer drives windows on the Vulkan platform", "[rendering][vulkan][server]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable: " << SDL_GetError());

    struct ShutdownGuard {
        ~ShutdownGuard()
        {
            RenderingServer::Instance().shutdown();
        }
    } guard;

    RenderingServer &server = RenderingServer::Instance();
    REQUIRE(server.init({ .platform = PlatformKind::Vulkan, .gpu_validation = true }));
    if (server.platformKind() != PlatformKind::Vulkan)
        SKIP("No Vulkan 1.3 device with the required features");

    WindowRid window = server.windowCreate({ .title = "server", .size = { 96, 64 } });
    REQUIRE(window.isValid());
    for (int frame = 0; frame < 3; ++frame) server.sync();

    REQUIRE(server.windowSetSize(window, { 160, 120 }));
    server.pollEvents();
    for (int frame = 0; frame < 3; ++frame) server.sync();

    server.windowFree(window);
    server.sync();

    REQUIRE(dynamic_cast<VulkanPlatform &>(server.platform()).validationErrorCount() == 0);
}

#endif
