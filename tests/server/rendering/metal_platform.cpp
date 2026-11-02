#if defined(ATMO_RENDERING_METAL)

#include <filesystem>
#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "server/rendering/api/metal/metal_platform.hpp"
#include "server/rendering/shader/builtin_shaders.hpp"

using namespace atmo::server::rendering;

namespace
{
    struct TemporaryDirectory {
        TemporaryDirectory() : path(std::filesystem::temp_directory_path() / "atmo-metal-pipeline-cache-test")
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

    void CreateCanvasPipeline(MetalPlatform &platform)
    {
        std::optional<ShaderBlob> blob = BuiltinShaderBlob(BuiltinShader::Canvas, ShaderTarget::Msl);
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

TEST_CASE("the Metal pipeline cache is written on shutdown and reopened", "[rendering][metal]")
{
    TemporaryDirectory cache;
    const RenderingSettings settings{ .pipeline_cache_dir = cache.path.string() };

    {
        MetalPlatform platform;
        if (!platform.initialize(settings))
            SKIP("No Metal 3 device with tier 2 argument buffers");
        CreateCanvasPipeline(platform);
        platform.shutdown();
    }

    const std::filesystem::path archive = cache.path / "metal-pipelines.bin";
    REQUIRE(std::filesystem::exists(archive));
    REQUIRE(std::filesystem::file_size(archive) > 0);

    MetalPlatform platform;
    REQUIRE(platform.initialize(settings));
    REQUIRE(platform.pipelineArchivePath() == archive);
    CreateCanvasPipeline(platform);
    platform.shutdown();
    REQUIRE(platform.validationErrorCount() == 0);
}

TEST_CASE("a corrupt Metal pipeline cache is replaced", "[rendering][metal]")
{
    TemporaryDirectory cache;
    std::filesystem::create_directories(cache.path);
    {
        std::FILE *file = std::fopen((cache.path / "metal-pipelines.bin").string().c_str(), "wb");
        REQUIRE(file != nullptr);
        std::fputs("not a binary archive", file);
        std::fclose(file);
    }

    MetalPlatform platform;
    if (!platform.initialize({ .pipeline_cache_dir = cache.path.string() }))
        SKIP("No Metal 3 device with tier 2 argument buffers");
    CreateCanvasPipeline(platform);
    platform.shutdown();
    REQUIRE(std::filesystem::file_size(cache.path / "metal-pipelines.bin") > std::string("not a binary archive").size());
}

TEST_CASE("Metal rejects shaders for other targets", "[rendering][metal]")
{
    MetalPlatform platform;
    if (!platform.initialize({}))
        SKIP("No Metal 3 device with tier 2 argument buffers");
    REQUIRE(platform.shaderTarget() == ShaderTarget::Msl);
    REQUIRE_THROWS_AS(platform.shaderModuleCreate({ .target = ShaderTarget::SpirV }), std::invalid_argument);
    REQUIRE_THROWS(platform.shaderModuleCreate({ .target = ShaderTarget::Msl, .code = std::as_bytes(std::span("not msl", 7)), .debug_name = "bad" }));
    platform.shutdown();
}

#endif
