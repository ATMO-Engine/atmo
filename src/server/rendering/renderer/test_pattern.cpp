#include "test_pattern.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>

#include "server/rendering/shader/builtin_shaders.hpp"
#include "spdlog/spdlog.h"

namespace atmo::server::rendering
{
    namespace
    {
        struct alignas(16) Constants {
            std::array<float, 4> rect;
            std::array<float, 4> color;
            std::uint32_t texture;
            std::uint32_t sampler;
            std::array<std::uint32_t, 2> padding;
        };

        constexpr std::array<std::uint16_t, 6> quad_indices{ 0, 1, 2, 2, 1, 3 };
    } // namespace

    TestPattern::TestPattern(RenderingPlatform &platform) : m_platform(platform) {}

    TestPattern::~TestPattern()
    {
        release();
    }

    bool TestPattern::isAvailable() const noexcept
    {
        return !m_failed;
    }

    bool TestPattern::ensureResources()
    {
        if (m_ready || m_failed)
            return m_ready;

        std::optional<ShaderBlob> blob = BuiltinShaderBlob(BuiltinShader::TestPattern, m_platform.shaderTarget());
        if (!blob) {
            spdlog::warn("The {} platform has no built-in test pattern shader", m_platform.name());
            m_failed = true;
            return false;
        }

        try {
            m_module = m_platform.shaderModuleCreate(*blob);

            m_texture = m_platform.textureCreate({ .size = { texture_size, texture_size }, .debug_name = "test pattern" });
            std::vector<std::byte> pixels(static_cast<std::size_t>(texture_size) * texture_size * 4);
            for (int y = 0; y < texture_size; ++y) {
                for (int x = 0; x < texture_size; ++x) {
                    const auto &texel = ((x / checker_size) + (y / checker_size)) % 2 == 0 ? light_texel : dark_texel;
                    std::byte *pixel = pixels.data() + ((static_cast<std::size_t>(y) * texture_size) + x) * 4;
                    for (std::size_t c = 0; c < 4; ++c) pixel[c] = static_cast<std::byte>(texel[c]);
                }
            }
            m_platform.textureUpload(m_texture, { .width = texture_size, .height = texture_size }, pixels);

            m_sampler = m_platform.samplerCreate({ .filter = Filter::Nearest, .address = AddressMode::ClampToEdge });

            m_indices = m_platform.bufferCreate(
                { .size = sizeof(quad_indices), .usage = BufferUsage::Index, .memory = MemoryType::Upload, .debug_name = "test pattern indices" });
            m_platform.bufferWrite(m_indices, 0, std::as_bytes(std::span(quad_indices)));
        } catch (const std::exception &e) {
            spdlog::error("Failed to create the test pattern resources: {}", e.what());
            release();
            m_failed = true;
            return false;
        }

        m_ready = true;
        return true;
    }

    PipelineHandle TestPattern::pipelineFor(TextureFormat format)
    {
        auto it = std::ranges::find(m_pipelines, format, &std::pair<TextureFormat, PipelineHandle>::first);
        if (it != m_pipelines.end())
            return it->second;

        PipelineHandle pipeline = m_platform.graphicsPipelineCreate(
            {
                .vertex = { .module = m_module, .entry_point = "vertexMain" },
                .fragment = { .module = m_module, .entry_point = "fragmentMain" },
                .color_format = format,
                .blend = BlendMode::Alpha,
                .debug_name = "test pattern",
            });
        m_pipelines.emplace_back(format, pipeline);
        return pipeline;
    }

    void TestPattern::render(std::span<const SwapchainHandle> swapchains)
    {
        if (swapchains.empty() || !ensureResources())
            return;

        CommandList &list = m_platform.commandListAcquire(QueueType::Graphics);
        std::vector<SwapchainHandle> acquired;
        acquired.reserve(swapchains.size());

        for (SwapchainHandle swapchain : swapchains) {
            TextureHandle image = m_platform.swapchainAcquire(swapchain);
            if (!image.isValid())
                continue;
            acquired.push_back(swapchain);

            const core::types::Vector2i size = m_platform.textureSize(image);
            const float side = quad_scale * static_cast<float>(std::min(size.x, size.y));
            const float width = 2.0f * side / static_cast<float>(size.x);
            const float height = 2.0f * side / static_cast<float>(size.y);

            const Constants constants{
                .rect = { -width / 2.0f, -height / 2.0f, width, height },
                .color = { 1.0f, 1.0f, 1.0f, 1.0f },
                .texture = m_platform.bindlessIndex(m_texture),
                .sampler = m_platform.bindlessIndex(m_sampler),
                .padding = {},
            };

            const PipelineHandle pipeline = pipelineFor(m_platform.swapchainFormat(swapchain));

            list.debugLabelPush("Test pattern");

            const TextureBarrier to_target{ .texture = image, .after = ResourceState::RenderTarget };
            list.barrier({ &to_target, 1 });

            const ColorAttachment attachment{ .texture = image, .load = LoadOp::Clear, .store = StoreOp::Store, .clear_color = clear_color };
            list.renderPassBegin({ .color_attachments = { &attachment, 1 }, .debug_name = "Test pattern" });
            list.viewportSet({ .width = static_cast<float>(size.x), .height = static_cast<float>(size.y) });
            list.scissorSet({ .width = size.x, .height = size.y });
            list.pipelineBind(pipeline);
            list.constantsPush(std::as_bytes(std::span(&constants, 1)));
            list.indexBufferBind(m_indices, IndexType::Uint16, 0);
            list.drawIndexed(static_cast<std::uint32_t>(quad_indices.size()), 1, 0, 0, 0);
            list.renderPassEnd();

            const TextureBarrier to_present{ .texture = image, .after = ResourceState::Present };
            list.barrier({ &to_present, 1 });

            list.debugLabelPop();
        }

        if (acquired.empty())
            return;

        CommandList *lists[] = { &list };
        m_platform.submit(lists);
        for (SwapchainHandle swapchain : acquired) m_platform.swapchainPresent(swapchain);
    }

    void TestPattern::release() noexcept
    {
        try {
            for (auto &[format, pipeline] : m_pipelines) m_platform.destroy(pipeline);
            m_pipelines.clear();
            if (m_indices.isValid())
                m_platform.destroy(m_indices);
            if (m_sampler.isValid())
                m_platform.destroy(m_sampler);
            if (m_texture.isValid())
                m_platform.destroy(m_texture);
            if (m_module.isValid())
                m_platform.destroy(m_module);
        } catch (const std::exception &e) {
            spdlog::error("Failed to release the test pattern resources: {}", e.what());
        }

        m_indices = {};
        m_sampler = {};
        m_texture = {};
        m_module = {};
        m_ready = false;
    }
} // namespace atmo::server::rendering
