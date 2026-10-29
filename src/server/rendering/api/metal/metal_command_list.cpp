#include "metal_command_list.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <stdexcept>

#include "metal_platform.hpp"
#include "metal_utils.hpp"

namespace atmo::server::rendering::metal
{
    MetalCommandList::MetalCommandList(MetalPlatform &platform) : m_platform(platform) {}

    void MetalCommandList::begin(MTL::CommandBuffer *commands)
    {
        m_commands = commands;
        m_render.reset();
        m_blit.reset();
        m_constants.fill(std::byte{ 0 });
        m_pipeline_bound = false;
        m_index_buffer = nullptr;
        m_labels.clear();
    }

    void MetalCommandList::end()
    {
        requireRecording("end");
        if (m_render)
            throw std::logic_error("command list ended inside a render pass");

        while (!m_labels.empty()) debugLabelPop();
        endBlit();
        m_commands = nullptr;
    }

    bool MetalCommandList::isRecording() const noexcept
    {
        return m_commands != nullptr;
    }

    bool MetalCommandList::inRenderPass() const noexcept
    {
        return static_cast<bool>(m_render);
    }

    MTL::BlitCommandEncoder *MetalCommandList::blitEncoder()
    {
        requireRecording("blitEncoder");
        if (m_render)
            throw std::logic_error("GPU transfers are not allowed inside a render pass");
        if (!m_blit)
            m_blit = NS::RetainPtr(m_commands->blitCommandEncoder());
        return m_blit.get();
    }

    void MetalCommandList::endBlit()
    {
        if (!m_blit)
            return;
        m_blit->endEncoding();
        m_blit.reset();
    }

    void MetalCommandList::barrier(std::span<const TextureBarrier> textures)
    {
        requireRecording("barrier");
        if (m_render)
            throw std::logic_error("barrier called inside a render pass");

        for (const TextureBarrier &barrier : textures) m_platform.transition(m_platform.texture(barrier.texture), barrier.texture, barrier.after);
    }

    void MetalCommandList::renderPassBegin(const RenderPassDesc &desc)
    {
        requireRecording("renderPassBegin");
        if (m_render)
            throw std::logic_error("renderPassBegin called inside a render pass");
        if (desc.color_attachments.empty())
            throw std::invalid_argument("renderPassBegin needs at least one color attachment");

        endBlit();

        MTL::RenderPassDescriptor *pass = MTL::RenderPassDescriptor::renderPassDescriptor();
        core::types::Vector2i extent{ INT32_MAX, INT32_MAX };
        std::vector<TextureHandle> attachments;
        attachments.reserve(desc.color_attachments.size());

        for (std::size_t i = 0; i < desc.color_attachments.size(); ++i) {
            const ColorAttachment &attachment = desc.color_attachments[i];
            const MetalPlatform::Texture &texture = m_platform.texture(attachment.texture);
            if (texture.state != ResourceState::RenderTarget)
                throw std::logic_error("renderPassBegin: color attachments must be transitioned to RenderTarget first");

            MTL::RenderPassColorAttachmentDescriptor *color = pass->colorAttachments()->object(i);
            color->setTexture(texture.texture.get());
            color->setLoadAction(ToMtlLoadAction(attachment.load));
            color->setStoreAction(ToMtlStoreAction(attachment.store));
            color->setClearColor(
                MTL::ClearColor::Make(attachment.clear_color[0], attachment.clear_color[1], attachment.clear_color[2], attachment.clear_color[3]));

            extent.x = std::min(extent.x, texture.desc.size.x);
            extent.y = std::min(extent.y, texture.desc.size.y);
            attachments.push_back(attachment.texture);
        }

        m_render = NS::RetainPtr(m_commands->renderCommandEncoder(pass));
        if (!m_render)
            throw std::runtime_error("renderCommandEncoder failed");
        if (!desc.debug_name.empty())
            m_render->setLabel(ToNsString(desc.debug_name));

        m_target_size = extent;
        m_pipeline_bound = false;
        m_render->setVertexBuffer(m_platform.m_bindless.get(), 0, MetalPlatform::bindless_slot);
        m_render->setFragmentBuffer(m_platform.m_bindless.get(), 0, MetalPlatform::bindless_slot);
        bindConstants();

        m_platform.prepareEncoderResources(m_resources, attachments);
        if (!m_resources.empty())
            m_render->useResources(m_resources.data(), m_resources.size(), MTL::ResourceUsageRead, MTL::RenderStageVertex | MTL::RenderStageFragment);
    }

    void MetalCommandList::renderPassEnd()
    {
        requireRenderPass("renderPassEnd");
        for (auto it = m_labels.rbegin(); it != m_labels.rend(); ++it) {
            if (*it == LabelTarget::RenderEncoder) {
                m_render->popDebugGroup();
                *it = LabelTarget::Closed;
            }
        }
        m_render->endEncoding();
        m_render.reset();
        m_pipeline_bound = false;
    }

    void MetalCommandList::pipelineBind(PipelineHandle pipeline)
    {
        requireRenderPass("pipelineBind");
        const MetalPlatform::Pipeline &entry = m_platform.m_pipelines.at(pipeline);
        m_render->setRenderPipelineState(entry.state.get());
        m_primitive = entry.primitive;
        m_pipeline_bound = true;
    }

    void MetalCommandList::bindConstants()
    {
        m_render->setVertexBytes(m_constants.data(), m_constants.size(), MetalPlatform::constants_slot);
        m_render->setFragmentBytes(m_constants.data(), m_constants.size(), MetalPlatform::constants_slot);
    }

    void MetalCommandList::constantsPush(std::span<const std::byte> data)
    {
        requireRecording("constantsPush");
        if (data.size() > MetalPlatform::push_constant_size || data.size() % 4 != 0)
            throw std::invalid_argument(std::format("constantsPush: size must be a multiple of 4 and at most {} bytes", MetalPlatform::push_constant_size));
        std::memcpy(m_constants.data(), data.data(), data.size());
        if (m_render)
            bindConstants();
    }

    void MetalCommandList::indexBufferBind(BufferHandle buffer, IndexType type, std::uint64_t offset)
    {
        requireRecording("indexBufferBind");
        const MetalPlatform::Buffer &entry = m_platform.m_buffers.at(buffer);
        if (!HasFlag(entry.desc.usage, BufferUsage::Index))
            throw std::logic_error("indexBufferBind: buffer was not created with Index usage");
        m_index_buffer = entry.buffer.get();
        m_index_type = type == IndexType::Uint16 ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32;
        m_index_offset = offset;
    }

    void MetalCommandList::viewportSet(const Viewport &viewport)
    {
        requireRenderPass("viewportSet");
        m_render->setViewport(
            MTL::Viewport{
                .originX = viewport.x,
                .originY = viewport.y,
                .width = viewport.width,
                .height = viewport.height,
                .znear = viewport.min_depth,
                .zfar = viewport.max_depth,
            });
    }

    void MetalCommandList::scissorSet(const Region2i &scissor)
    {
        requireRenderPass("scissorSet");
        const int x = std::clamp(scissor.x, 0, m_target_size.x);
        const int y = std::clamp(scissor.y, 0, m_target_size.y);
        const int right = std::clamp(scissor.x + std::max(scissor.width, 0), x, m_target_size.x);
        const int bottom = std::clamp(scissor.y + std::max(scissor.height, 0), y, m_target_size.y);
        m_render->setScissorRect(
            MTL::ScissorRect{
                .x = static_cast<NS::UInteger>(x),
                .y = static_cast<NS::UInteger>(y),
                .width = static_cast<NS::UInteger>(right - x),
                .height = static_cast<NS::UInteger>(bottom - y),
            });
    }

    void MetalCommandList::draw(std::uint32_t vertices, std::uint32_t instances, std::uint32_t first_vertex, std::uint32_t first_instance)
    {
        requireRenderPass("draw");
        if (!m_pipeline_bound)
            throw std::logic_error("draw called without a bound pipeline");
        if (vertices == 0 || instances == 0)
            return;
        m_render->drawPrimitives(m_primitive, first_vertex, vertices, instances, first_instance);
    }

    void MetalCommandList::drawIndexed(
        std::uint32_t indices, std::uint32_t instances, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance)
    {
        requireRenderPass("drawIndexed");
        if (!m_pipeline_bound)
            throw std::logic_error("drawIndexed called without a bound pipeline");
        if (!m_index_buffer)
            throw std::logic_error("drawIndexed called without an index buffer");
        if (indices == 0 || instances == 0)
            return;
        const std::uint64_t index_size = m_index_type == MTL::IndexTypeUInt16 ? 2 : 4;
        m_render->drawIndexedPrimitives(
            m_primitive, indices, m_index_type, m_index_buffer, m_index_offset + (first_index * index_size), instances, vertex_offset, first_instance);
    }

    void MetalCommandList::debugLabelPush(std::string_view label)
    {
        requireRecording("debugLabelPush");
        NS::String *name = ToNsString(label);
        if (m_render) {
            m_render->pushDebugGroup(name);
            m_labels.push_back(LabelTarget::RenderEncoder);
        } else {
            endBlit();
            m_commands->pushDebugGroup(name);
            m_labels.push_back(LabelTarget::CommandBuffer);
        }
    }

    void MetalCommandList::debugLabelPop()
    {
        requireRecording("debugLabelPop");
        if (m_labels.empty())
            throw std::logic_error("debugLabelPop called without a matching push");

        const LabelTarget target = m_labels.back();
        if (target == LabelTarget::CommandBuffer && m_render)
            throw std::logic_error("debugLabelPop: the label was pushed outside the open render pass");
        m_labels.pop_back();

        if (target == LabelTarget::RenderEncoder) {
            m_render->popDebugGroup();
        } else if (target == LabelTarget::CommandBuffer) {
            endBlit();
            m_commands->popDebugGroup();
        }
    }

    void MetalCommandList::requireRecording(const char *call) const
    {
        if (!m_commands)
            throw std::logic_error(std::format("{} called on a command list that is not recording", call));
    }

    void MetalCommandList::requireRenderPass(const char *call) const
    {
        requireRecording(call);
        if (!m_render)
            throw std::logic_error(std::format("{} called outside a render pass", call));
    }
} // namespace atmo::server::rendering::metal
