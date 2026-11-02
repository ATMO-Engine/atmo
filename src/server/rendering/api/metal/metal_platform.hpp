#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "metal_command_list.hpp"
#include "metal_includes.hpp"
#include "server/rendering/platform/handle_pool.hpp"
#include "server/rendering/platform/rendering_platform.hpp"

namespace atmo::server::rendering
{
    class MetalPlatform final : public RenderingPlatform
    {
    public:
        static constexpr std::uint32_t frames_in_flight = 2;
        static constexpr std::uint32_t push_constant_size = 128;
        static constexpr std::size_t staging_size = 16ull * 1024 * 1024;
        static constexpr std::uint32_t max_bindless_textures = 16384;
        static constexpr std::uint32_t max_bindless_samplers = 64;
        static constexpr std::uint32_t max_bindless_buffers = 4096;
        static constexpr NS::UInteger constants_slot = 0;
        static constexpr NS::UInteger bindless_slot = 30;

        MetalPlatform();
        ~MetalPlatform() override;

        MetalPlatform(const MetalPlatform &) = delete;
        MetalPlatform &operator=(const MetalPlatform &) = delete;

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

        [[nodiscard]] bool validationEnabled() const noexcept;
        [[nodiscard]] std::uint32_t validationErrorCount() const noexcept;
        [[nodiscard]] bool usesResidencySets() const noexcept;
        [[nodiscard]] const std::filesystem::path &pipelineArchivePath() const noexcept;

    private:
        friend class metal::MetalCommandList;

        static constexpr std::uint32_t invalid_index = ~0u;

        struct Buffer {
            NS::SharedPtr<MTL::Buffer> buffer;
            BufferDesc desc;
            std::uint32_t bindless = invalid_index;
        };

        struct Texture {
            NS::SharedPtr<MTL::Texture> texture;
            TextureDesc desc;
            ResourceState state = ResourceState::Undefined;
            std::uint32_t bindless = invalid_index;
            bool swapchain_image = false;
        };

        struct Sampler {
            NS::SharedPtr<MTL::SamplerState> sampler;
            std::uint32_t bindless = invalid_index;
        };

        struct Pipeline {
            NS::SharedPtr<MTL::RenderPipelineState> state;
            MTL::PrimitiveType primitive = MTL::PrimitiveTypeTriangle;
        };

        struct Swapchain {
            NS::SharedPtr<CA::MetalLayer> layer;
            NS::SharedPtr<CA::MetalDrawable> drawable;
            TextureHandle image;
            core::types::Vector2i size{};
            bool acquired = false;
            bool presented = false;
        };

        struct Frame {
            NS::SharedPtr<MTL::CommandBuffer> commands;
            NS::SharedPtr<MTL::CommandBuffer> uploads;
            NS::SharedPtr<MTL::Buffer> staging;
            std::size_t staging_offset = 0;
            std::uint64_t number = 0;
        };

        struct Deletion {
            std::uint64_t frame;
            std::function<void()> destroy;
        };

        struct StagingSpan {
            MTL::Buffer *buffer;
            std::size_t offset;
            std::byte *data;
        };

        bool createDevice(const RenderingSettings &settings);
        void createBindlessTable();
        void createResidency();
        void openPipelineArchive(const RenderingSettings &settings);
        void savePipelineArchive();

        Frame &prepareFrame(std::uint64_t number);
        MTL::CommandBuffer *newCommandBuffer(std::string_view label);
        void retire(NS::SharedPtr<MTL::CommandBuffer> &commands);
        MTL::BlitCommandEncoder *transferEncoder();
        void flushUploads(Frame &frame);
        void commitResidency();
        StagingSpan stagingAllocate(std::size_t size);
        void defer(std::function<void()> destroy);
        void collectGarbage(std::uint64_t completed_frame) noexcept;
        void waitIdle() noexcept;

        void transition(Texture &texture, TextureHandle handle, ResourceState after);
        void makeResident(MTL::Allocation *allocation);
        void evict(MTL::Allocation *allocation);
        void writeBindless(std::size_t offset, std::uint64_t value) noexcept;
        void prepareEncoderResources(std::vector<const MTL::Resource *> &resources, std::span<const TextureHandle> attachments);
        void submitFrame(std::span<CommandList *const> lists);

        Texture &texture(TextureHandle handle);
        const Texture &texture(TextureHandle handle) const;

        PlatformCaps m_caps;
        bool m_initialized = false;
        bool m_validation = false;

        NS::SharedPtr<MTL::Device> m_device;
        NS::SharedPtr<MTL::CommandQueue> m_queue;
        NS::SharedPtr<MTL::Buffer> m_bindless;
        std::vector<std::uint32_t> m_free_texture_indices;
        std::vector<std::uint32_t> m_free_sampler_indices;
        std::vector<std::uint32_t> m_free_buffer_indices;

        NS::SharedPtr<MTL::ResidencySet> m_residency;
        bool m_residency_dirty = false;
        std::vector<const MTL::Resource *> m_resident;

        NS::SharedPtr<MTL::BinaryArchive> m_archive;
        std::filesystem::path m_archive_path;
        bool m_archive_dirty = false;

        std::array<Frame, frames_in_flight> m_frames{};
        std::uint64_t m_frame_number = 0;
        std::uint64_t m_completed_frame = 0;
        bool m_in_frame = false;
        bool m_submitted = false;
        NS::AutoreleasePool *m_frame_pool = nullptr;
        NS::SharedPtr<MTL::BlitCommandEncoder> m_upload_encoder;
        std::unique_ptr<metal::MetalCommandList> m_command_list;
        std::vector<Deletion> m_deletions;
        std::unordered_map<std::uint32_t, std::uint64_t> m_written;

        HandlePool<BufferHandle, Buffer> m_buffers;
        HandlePool<TextureHandle, Texture> m_textures;
        HandlePool<SamplerHandle, Sampler> m_samplers;
        HandlePool<ShaderModuleHandle, NS::SharedPtr<MTL::Library>> m_modules;
        HandlePool<PipelineHandle, Pipeline> m_pipelines;
        HandlePool<SwapchainHandle, Swapchain> m_swapchains;

        std::uint32_t m_gpu_errors = 0;
    };
} // namespace atmo::server::rendering
