#pragma once

#include <memory>
#include <vector>

#include "server/rendering/platform/rendering_platform.hpp"
#include "vulkan_includes.hpp"

namespace atmo::server::rendering
{
    class VulkanPlatform;
}

namespace atmo::server::rendering::vulkan
{
    class VulkanCommandList final : public CommandList
    {
    public:
        explicit VulkanCommandList(VulkanPlatform &platform);
        ~VulkanCommandList() override;

        void begin(VkCommandBuffer commands);
        VkCommandBuffer end();
        [[nodiscard]] bool isRecording() const noexcept;
        [[nodiscard]] bool inRenderPass() const noexcept;
        [[nodiscard]] VkCommandBuffer commands() const noexcept;

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

    private:
        void requireRecording(const char *call) const;
        void requireRenderPass(const char *call) const;

        VulkanPlatform &m_platform;
        VkCommandBuffer m_commands = VK_NULL_HANDLE;
        bool m_in_render_pass = false;
        bool m_pipeline_bound = false;
        std::uint32_t m_label_depth = 0;

        struct GpuZones;
        std::unique_ptr<GpuZones> m_zones;
    };
} // namespace atmo::server::rendering::vulkan
