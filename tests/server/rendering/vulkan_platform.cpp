#if defined(ATMO_RENDERING_VULKAN)

#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "offscreen_video.hpp"
#include "server/rendering/api/vulkan/vulkan_platform.hpp"
#include "server/rendering/shader/builtin_shaders.hpp"

using namespace atmo::server::rendering;

namespace
{
    struct TemporaryDirectory {
        TemporaryDirectory() : path(std::filesystem::temp_directory_path() / "atmo-vulkan-pipeline-cache-test")
        {
            std::filesystem::remove_all(path);
        }

        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }

        std::filesystem::path path;
    };

    void CreateCanvasPipeline(VulkanPlatform &platform)
    {
        std::optional<ShaderBlob> blob = BuiltinShaderBlob(BuiltinShader::Canvas, ShaderTarget::SpirV);
        REQUIRE(blob.has_value());
        ShaderModuleHandle module = platform.shaderModuleCreate(*blob);
        PipelineHandle pipeline = platform.graphicsPipelineCreate(
            {
                .vertex = { .module = module, .entry_point = "quadVertexMain" },
                .fragment = { .module = module, .entry_point = "canvasFragmentMain" },
                .color_format = TextureFormat::Rgba8Unorm,
            });
        REQUIRE(pipeline.isValid());
        platform.destroy(pipeline);
        platform.destroy(module);
    }
} // namespace

TEST_CASE("the Vulkan pipeline cache is written on shutdown and reopened", "[rendering][vulkan]")
{
    OffscreenVideo video;
    if (!video.available)
        SKIP("SDL offscreen video driver unavailable");

    TemporaryDirectory cache;
    const RenderingSettings settings{ .pipeline_cache_dir = cache.path.string() };
    const std::filesystem::path file = cache.path / "vulkan-pipelines.bin";

    {
        VulkanPlatform platform;
        if (!platform.initialize(settings))
            SKIP("No Vulkan 1.3 device with the required features");
        REQUIRE_FALSE(platform.pipelineCacheLoaded());
        CreateCanvasPipeline(platform);
        platform.shutdown();
    }
    REQUIRE(std::filesystem::exists(file));
    REQUIRE(std::filesystem::file_size(file) > 0);

    {
        VulkanPlatform platform;
        REQUIRE(platform.initialize(settings));
        REQUIRE(platform.pipelineCachePath() == file);
        REQUIRE(platform.pipelineCacheLoaded());
        CreateCanvasPipeline(platform);
        platform.shutdown();
    }

    {
        std::FILE *corrupt = std::fopen(file.string().c_str(), "wb");
        REQUIRE(corrupt != nullptr);
        std::fputs("not a pipeline cache", corrupt);
        std::fclose(corrupt);
    }
    VulkanPlatform platform;
    REQUIRE(platform.initialize(settings));
    REQUIRE_FALSE(platform.pipelineCacheLoaded());
    CreateCanvasPipeline(platform);
    platform.shutdown();
    REQUIRE(std::filesystem::file_size(file) > std::string("not a pipeline cache").size());
}

#endif
