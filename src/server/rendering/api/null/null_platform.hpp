#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "server/rendering/platform/handle_pool.hpp"
#include "server/rendering/platform/rendering_platform.hpp"

namespace atmo::server::rendering
{
    class NullCommandList final : public CommandList
    {
    public:
        void barrier(std::span<const TextureBarrier> textures) override;
        void renderPassBegin(const RenderPassDesc &desc) override;
        void renderPassEnd() override;
        void pipelineBind(PipelineHandle pipeline) override;
        void constantsPush(std::span<const std::byte> data) override;
        void indexBufferBind(BufferHandle buffer, IndexType type, std::uint64_t offset) override;
        void viewportSet(const Viewport &viewport) override;
        void scissorSet(const Region2i &scissor) override;
        void draw(std::uint32_t vertices, std::uint32_t instances, std::uint32_t first_vertex, std::uint32_t first_instance) override;
        void drawIndexed(
            std::uint32_t indices, std::uint32_t instances, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance) override;
        void debugLabelPush(std::string_view label) override;
        void debugLabelPop() override;

        void reset();
        [[nodiscard]] bool isIdle() const noexcept;
        [[nodiscard]] std::uint64_t drawCount() const noexcept;
        [[nodiscard]] std::uint64_t renderPassCount() const noexcept;

    private:
        void requireRenderPass(const char *call) const;

        std::uint64_t m_draws = 0;
        std::uint64_t m_render_passes = 0;
        std::uint32_t m_label_depth = 0;
        bool m_in_render_pass = false;
        bool m_pipeline_bound = false;
    };

    class NullPlatform final : public RenderingPlatform
    {
    public:
        NullPlatform();

        [[nodiscard]] PlatformKind kind() const noexcept override;
        [[nodiscard]] std::string_view name() const noexcept override;

        bool initialize(const RenderingSettings &settings) override;
        void shutdown() override;

        [[nodiscard]] SDL_WindowFlags windowFlags() const noexcept override;
        [[nodiscard]] const PlatformCaps &caps() const noexcept override;
        [[nodiscard]] ShaderTarget shaderTarget() const noexcept override;

        SwapchainHandle swapchainCreate(const NativeWindow &window, const SwapchainDesc &desc) override;
        void swapchainResize(SwapchainHandle swapchain, core::types::Vector2i size) override;
        void swapchainDestroy(SwapchainHandle swapchain) override;
        [[nodiscard]] TextureFormat swapchainFormat(SwapchainHandle swapchain) const override;
        TextureHandle swapchainAcquire(SwapchainHandle swapchain) override;
        void swapchainPresent(SwapchainHandle swapchain) override;

        BufferHandle bufferCreate(const BufferDesc &desc) override;
        void bufferWrite(BufferHandle buffer, std::uint64_t offset, std::span<const std::byte> data) override;
        TextureHandle textureCreate(const TextureDesc &desc) override;
        void textureUpload(TextureHandle texture, const Region2i &region, std::span<const std::byte> pixels) override;
        std::vector<std::byte> textureReadback(TextureHandle texture) override;
        [[nodiscard]] core::types::Vector2i textureSize(TextureHandle texture) const override;
        SamplerHandle samplerCreate(const SamplerDesc &desc) override;
        ShaderModuleHandle shaderModuleCreate(const ShaderBlob &blob) override;
        PipelineHandle graphicsPipelineCreate(const GraphicsPipelineDesc &desc) override;

        void destroy(BufferHandle buffer) override;
        void destroy(TextureHandle texture) override;
        void destroy(SamplerHandle sampler) override;
        void destroy(ShaderModuleHandle module) override;
        void destroy(PipelineHandle pipeline) override;

        [[nodiscard]] std::uint32_t bindlessIndex(TextureHandle texture) const override;
        [[nodiscard]] std::uint32_t bindlessIndex(SamplerHandle sampler) const override;
        [[nodiscard]] std::uint32_t bindlessIndex(BufferHandle buffer) const override;

        void frameBegin() override;
        CommandList &commandListAcquire(QueueType queue) override;
        void submit(std::span<CommandList *const> lists) override;
        void frameEnd() override;

        [[nodiscard]] std::size_t swapchainCount() const noexcept;
        [[nodiscard]] std::optional<core::types::Vector2i> swapchainSize(SwapchainHandle swapchain) const;
        [[nodiscard]] std::vector<core::types::Vector2i> swapchainSizes();
        [[nodiscard]] std::uint64_t frameCount() const noexcept;
        [[nodiscard]] std::uint64_t presentCount() const noexcept;
        [[nodiscard]] std::uint64_t drawCount() const noexcept;
        [[nodiscard]] std::size_t liveObjectCount() const noexcept;

    private:
        struct Swapchain {
            core::types::Vector2i size;
            TextureHandle image;
            bool acquired = false;
        };

        struct Buffer {
            BufferDesc desc;
            std::vector<std::byte> data;
        };

        struct Texture {
            TextureDesc desc;
            std::vector<std::byte> pixels;
            bool swapchain_image = false;
        };

        void requireFrame(const char *call) const;

        PlatformCaps m_caps;
        HandlePool<SwapchainHandle, Swapchain> m_swapchains;
        HandlePool<BufferHandle, Buffer> m_buffers;
        HandlePool<TextureHandle, Texture> m_textures;
        HandlePool<SamplerHandle, SamplerDesc> m_samplers;
        HandlePool<ShaderModuleHandle, ShaderTarget> m_modules;
        HandlePool<PipelineHandle, GraphicsPipelineDesc> m_pipelines;
        NullCommandList m_command_list;
        std::uint64_t m_frame_count = 0;
        std::uint64_t m_present_count = 0;
        std::uint64_t m_draw_count = 0;
        bool m_in_frame = false;
        bool m_submitted = false;
    };
} // namespace atmo::server::rendering
