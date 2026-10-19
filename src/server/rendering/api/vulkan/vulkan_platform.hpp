#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "server/rendering/platform/handle_pool.hpp"
#include "server/rendering/platform/rendering_platform.hpp"
#include "vulkan_command_list.hpp"
#include "vulkan_includes.hpp"

#if defined(ATMO_PROFILER_TRACY)
namespace tracy
{
    class VkCtx;
}
#endif

namespace atmo::server::rendering
{
    class VulkanPlatform final : public RenderingPlatform
    {
    public:
        static constexpr std::uint32_t frames_in_flight = 2;
        static constexpr std::uint32_t push_constant_size = 128;
        static constexpr VkDeviceSize staging_size = 16ull * 1024 * 1024;

        VulkanPlatform();
        ~VulkanPlatform() override;

        VulkanPlatform(const VulkanPlatform &) = delete;
        VulkanPlatform &operator=(const VulkanPlatform &) = delete;

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
        [[nodiscard]] std::uint32_t validationWarningCount() const noexcept;

    private:
        friend class vulkan::VulkanCommandList;

        static constexpr std::uint32_t invalid_index = ~0u;

        struct Buffer {
            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            void *mapped = nullptr;
            BufferDesc desc;
            std::uint32_t bindless = invalid_index;
        };

        struct Texture {
            VkImage image = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            VkFormat format = VK_FORMAT_UNDEFINED;
            TextureDesc desc;
            ResourceState state = ResourceState::Undefined;
            std::uint32_t bindless = invalid_index;
            bool swapchain_image = false;
        };

        struct Sampler {
            VkSampler sampler = VK_NULL_HANDLE;
            std::uint32_t bindless = invalid_index;
        };

        struct Swapchain {
            SDL_Window *window = nullptr;
            VkSurfaceKHR surface = VK_NULL_HANDLE;
            VkSwapchainKHR swapchain = VK_NULL_HANDLE;
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkExtent2D extent{};
            core::types::Vector2i requested_size{};
            bool vsync = true;
            std::vector<TextureHandle> images;
            std::vector<VkSemaphore> present_semaphores;
            std::array<VkSemaphore, frames_in_flight> acquire_semaphores{};
            std::uint32_t image_index = 0;
            bool acquired = false;
            bool waited = false;
            bool presented = false;
            bool needs_recreate = false;
            bool sdl_surface = true;
        };

        struct Frame {
            VkCommandPool pool = VK_NULL_HANDLE;
            VkCommandBuffer commands = VK_NULL_HANDLE;
            VkCommandBuffer uploads = VK_NULL_HANDLE;
            VkFence fence = VK_NULL_HANDLE;
            VkBuffer staging = VK_NULL_HANDLE;
            VmaAllocation staging_allocation = VK_NULL_HANDLE;
            std::byte *staging_mapped = nullptr;
            VkDeviceSize staging_offset = 0;
            std::uint64_t number = 0;
            bool uploads_recording = false;
        };

        struct Deletion {
            std::uint64_t frame;
            std::function<void()> destroy;
        };

        struct StagingSpan {
            VkBuffer buffer;
            VkDeviceSize offset;
            std::byte *data;
        };

        bool createInstance(const RenderingSettings &settings);
        bool selectPhysicalDevice();
        bool createDevice();
        void createAllocator();
        void createBindlessTable();
        void createFrames();
        void destroyFrames() noexcept;

        Frame &prepareFrame(std::uint64_t number);
        VkCommandBuffer transferCommands();
        StagingSpan stagingAllocate(VkDeviceSize size);
        void defer(std::function<void()> destroy);
        void collectGarbage(std::uint64_t completed_frame) noexcept;
        void waitIdle() noexcept;
        void setDebugName(VkObjectType type, std::uint64_t handle, std::string_view name) const;

        void transition(VkCommandBuffer commands, Texture &texture, ResourceState after) const;

        void swapchainBuild(Swapchain &swapchain);
        void swapchainReleaseImages(Swapchain &swapchain) noexcept;
        void surfaceDestroy(const Swapchain &swapchain) noexcept;
        bool surfaceCreate(const NativeWindow &window, Swapchain &swapchain);
        void submitFrame(std::span<CommandList *const> lists);

        Texture &texture(TextureHandle handle);
        const Texture &texture(TextureHandle handle) const;

        PlatformCaps m_caps;
        bool m_initialized = false;
        bool m_sdl_library_loaded = false;
        bool m_validation = false;
        bool m_debug_utils = false;
        bool m_portability_subset = false;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_messenger = VK_NULL_HANDLE;
        VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_properties{};
        VkDevice m_device = VK_NULL_HANDLE;
        std::uint32_t m_queue_family = 0;
        VkQueue m_queue = VK_NULL_HANDLE;
        VmaAllocator m_allocator = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_bindless_layout = VK_NULL_HANDLE;
        VkDescriptorPool m_bindless_pool = VK_NULL_HANDLE;
        VkDescriptorSet m_bindless_set = VK_NULL_HANDLE;
        VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
        std::vector<std::uint32_t> m_free_texture_indices;
        std::vector<std::uint32_t> m_free_sampler_indices;
        std::vector<std::uint32_t> m_free_buffer_indices;

        std::array<Frame, frames_in_flight> m_frames{};
        std::uint64_t m_frame_number = 0;
        std::uint64_t m_completed_frame = 0;
        bool m_in_frame = false;
        bool m_submitted = false;
        std::unique_ptr<vulkan::VulkanCommandList> m_command_list;
        std::vector<Deletion> m_deletions;

        HandlePool<BufferHandle, Buffer> m_buffers;
        HandlePool<TextureHandle, Texture> m_textures;
        HandlePool<SamplerHandle, Sampler> m_samplers;
        HandlePool<ShaderModuleHandle, VkShaderModule> m_modules;
        HandlePool<PipelineHandle, VkPipeline> m_pipelines;
        HandlePool<SwapchainHandle, Swapchain> m_swapchains;

        std::uint32_t m_validation_errors = 0;
        std::uint32_t m_validation_warnings = 0;

        struct Profiler {
#if defined(ATMO_PROFILER_TRACY)
            tracy::VkCtx *context = nullptr;
#endif
        };
        Profiler m_profiler;

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT *data,
            void *user_data);
    };
} // namespace atmo::server::rendering
