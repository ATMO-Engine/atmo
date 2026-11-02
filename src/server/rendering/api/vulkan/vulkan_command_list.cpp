#include "vulkan_command_list.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include "vulkan_platform.hpp"
#include "vulkan_utils.hpp"

#if defined(ATMO_PROFILER_TRACY)
#include <tracy/TracyVulkan.hpp>
#endif

namespace atmo::server::rendering::vulkan
{
    struct VulkanCommandList::GpuZones {
#if defined(ATMO_PROFILER_TRACY)
        std::vector<std::unique_ptr<tracy::VkCtxScope>> scopes;
#endif
    };

    VulkanCommandList::VulkanCommandList(VulkanPlatform &platform) : m_platform(platform), m_zones(std::make_unique<GpuZones>()) {}

    VulkanCommandList::~VulkanCommandList() = default;

    void VulkanCommandList::begin(VkCommandBuffer commands)
    {
        const VkCommandBufferBeginInfo info{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
        Check(vkBeginCommandBuffer(commands, &info), "vkBeginCommandBuffer");

        m_commands = commands;
        m_in_render_pass = false;
        m_pipeline_bound = false;
        m_label_depth = 0;

        for (VkPipelineBindPoint bind_point : { VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE_BIND_POINT_COMPUTE })
            vkCmdBindDescriptorSets(m_commands, bind_point, m_platform.m_pipeline_layout, 0, 1, &m_platform.m_bindless_set, 0, nullptr);

        static constexpr std::array<std::byte, VulkanPlatform::push_constant_size> zero_constants{};
        vkCmdPushConstants(m_commands, m_platform.m_pipeline_layout, VK_SHADER_STAGE_ALL, 0, VulkanPlatform::push_constant_size, zero_constants.data());
    }

    VkCommandBuffer VulkanCommandList::end()
    {
        requireRecording("end");
        if (m_in_render_pass)
            throw std::logic_error("command list ended inside a render pass");

        while (m_label_depth > 0) debugLabelPop();

        VkCommandBuffer commands = m_commands;
        m_commands = VK_NULL_HANDLE;
        Check(vkEndCommandBuffer(commands), "vkEndCommandBuffer");
        return commands;
    }

    bool VulkanCommandList::isRecording() const noexcept
    {
        return m_commands != VK_NULL_HANDLE;
    }

    bool VulkanCommandList::inRenderPass() const noexcept
    {
        return m_in_render_pass;
    }

    VkCommandBuffer VulkanCommandList::commands() const noexcept
    {
        return m_commands;
    }

    void VulkanCommandList::barrier(std::span<const TextureBarrier> textures)
    {
        requireRecording("barrier");
        if (m_in_render_pass)
            throw std::logic_error("barrier called inside a render pass");

        for (const TextureBarrier &barrier : textures) m_platform.transition(m_commands, m_platform.texture(barrier.texture), barrier.after);
    }

    void VulkanCommandList::renderPassBegin(const RenderPassDesc &desc)
    {
        requireRecording("renderPassBegin");
        if (m_in_render_pass)
            throw std::logic_error("renderPassBegin called inside a render pass");
        if (desc.color_attachments.empty())
            throw std::invalid_argument("renderPassBegin needs at least one color attachment");

        std::vector<VkRenderingAttachmentInfo> attachments;
        attachments.reserve(desc.color_attachments.size());
        VkExtent2D extent{ UINT32_MAX, UINT32_MAX };

        for (const ColorAttachment &attachment : desc.color_attachments) {
            const VulkanPlatform::Texture &texture = m_platform.texture(attachment.texture);
            if (texture.state != ResourceState::RenderTarget)
                throw std::logic_error("renderPassBegin: color attachments must be transitioned to RenderTarget first");

            VkRenderingAttachmentInfo info{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = texture.view,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = ToVkLoadOp(attachment.load),
                .storeOp = ToVkStoreOp(attachment.store),
            };
            std::copy(attachment.clear_color.begin(), attachment.clear_color.end(), info.clearValue.color.float32);
            attachments.push_back(info);

            extent.width = std::min(extent.width, static_cast<std::uint32_t>(texture.desc.size.x));
            extent.height = std::min(extent.height, static_cast<std::uint32_t>(texture.desc.size.y));
        }

        const VkRenderingInfo info{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { .offset = { 0, 0 }, .extent = extent },
            .layerCount = 1,
            .colorAttachmentCount = static_cast<std::uint32_t>(attachments.size()),
            .pColorAttachments = attachments.data(),
        };
        vkCmdBeginRendering(m_commands, &info);
        m_in_render_pass = true;
        m_pipeline_bound = false;
    }

    void VulkanCommandList::renderPassEnd()
    {
        requireRenderPass("renderPassEnd");
        vkCmdEndRendering(m_commands);
        m_in_render_pass = false;
        m_pipeline_bound = false;
    }

    void VulkanCommandList::pipelineBind(PipelineHandle pipeline)
    {
        requireRenderPass("pipelineBind");
        vkCmdBindPipeline(m_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_platform.m_pipelines.at(pipeline));
        m_pipeline_bound = true;
    }

    void VulkanCommandList::constantsPush(std::span<const std::byte> data)
    {
        requireRecording("constantsPush");
        if (data.size() > VulkanPlatform::push_constant_size || data.size() % 4 != 0)
            throw std::invalid_argument(std::format("constantsPush: size must be a multiple of 4 and at most {} bytes", VulkanPlatform::push_constant_size));
        vkCmdPushConstants(m_commands, m_platform.m_pipeline_layout, VK_SHADER_STAGE_ALL, 0, static_cast<std::uint32_t>(data.size()), data.data());
    }

    void VulkanCommandList::indexBufferBind(BufferHandle buffer, IndexType type, std::uint64_t offset)
    {
        requireRecording("indexBufferBind");
        const VulkanPlatform::Buffer &entry = m_platform.m_buffers.at(buffer);
        if (!HasFlag(entry.desc.usage, BufferUsage::Index))
            throw std::logic_error("indexBufferBind: buffer was not created with Index usage");
        vkCmdBindIndexBuffer(m_commands, entry.buffer, offset, type == IndexType::Uint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    }

    void VulkanCommandList::viewportSet(const Viewport &viewport)
    {
        requireRenderPass("viewportSet");
        const VkViewport flipped{
            .x = viewport.x,
            .y = viewport.y + viewport.height,
            .width = viewport.width,
            .height = -viewport.height,
            .minDepth = viewport.min_depth,
            .maxDepth = viewport.max_depth,
        };
        vkCmdSetViewport(m_commands, 0, 1, &flipped);
    }

    void VulkanCommandList::scissorSet(const Region2i &scissor)
    {
        requireRenderPass("scissorSet");
        const VkRect2D rect{
            .offset = { std::max(scissor.x, 0), std::max(scissor.y, 0) },
            .extent = { static_cast<std::uint32_t>(std::max(scissor.width, 0)), static_cast<std::uint32_t>(std::max(scissor.height, 0)) },
        };
        vkCmdSetScissor(m_commands, 0, 1, &rect);
    }

    void VulkanCommandList::draw(std::uint32_t vertices, std::uint32_t instances, std::uint32_t first_vertex, std::uint32_t first_instance)
    {
        requireRenderPass("draw");
        if (!m_pipeline_bound)
            throw std::logic_error("draw called without a bound pipeline");
        vkCmdDraw(m_commands, vertices, instances, first_vertex, first_instance);
    }

    void VulkanCommandList::drawIndexed(
        std::uint32_t indices, std::uint32_t instances, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance)
    {
        requireRenderPass("drawIndexed");
        if (!m_pipeline_bound)
            throw std::logic_error("drawIndexed called without a bound pipeline");
        vkCmdDrawIndexed(m_commands, indices, instances, first_index, vertex_offset, first_instance);
    }

    void VulkanCommandList::debugLabelPush(std::string_view label)
    {
        requireRecording("debugLabelPush");
        ++m_label_depth;

        if (m_platform.m_debug_utils) {
            const std::string name(label);
            const VkDebugUtilsLabelEXT info{ .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, .pLabelName = name.c_str() };
            vkCmdBeginDebugUtilsLabelEXT(m_commands, &info);
        }

#if defined(ATMO_PROFILER_TRACY)
        if (m_platform.m_profiler.context) {
            static constexpr std::string_view file = __FILE__;
            static constexpr std::string_view function = "debugLabelPush";
            m_zones->scopes.push_back(
                std::make_unique<tracy::VkCtxScope>(
                    m_platform.m_profiler.context,
                    static_cast<std::uint32_t>(__LINE__),
                    file.data(),
                    file.size(),
                    function.data(),
                    function.size(),
                    label.data(),
                    label.size(),
                    m_commands,
                    true));
        }
#endif
    }

    void VulkanCommandList::debugLabelPop()
    {
        requireRecording("debugLabelPop");
        if (m_label_depth == 0)
            throw std::logic_error("debugLabelPop called without a matching push");
        --m_label_depth;

#if defined(ATMO_PROFILER_TRACY)
        if (!m_zones->scopes.empty())
            m_zones->scopes.pop_back();
#endif

        if (m_platform.m_debug_utils)
            vkCmdEndDebugUtilsLabelEXT(m_commands);
    }

    void VulkanCommandList::requireRecording(const char *call) const
    {
        if (m_commands == VK_NULL_HANDLE)
            throw std::logic_error(std::format("{} called on a command list that is not recording", call));
    }

    void VulkanCommandList::requireRenderPass(const char *call) const
    {
        requireRecording(call);
        if (!m_in_render_pass)
            throw std::logic_error(std::format("{} called outside a render pass", call));
    }
} // namespace atmo::server::rendering::vulkan
