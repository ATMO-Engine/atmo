#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "metal_includes.hpp"
#include "server/rendering/platform/rendering_platform.hpp"

namespace atmo::server::rendering
{
    class MetalPlatform;
}

namespace atmo::server::rendering::metal
{
    class MetalCommandList final : public CommandList
    {
    public:
        explicit MetalCommandList(MetalPlatform &platform);

        void begin(MTL::CommandBuffer *commands);
        void end();
        [[nodiscard]] bool isRecording() const noexcept;
        [[nodiscard]] bool inRenderPass() const noexcept;
        [[nodiscard]] MTL::BlitCommandEncoder *blitEncoder();

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
        enum class LabelTarget : std::uint8_t {
            CommandBuffer,
            RenderEncoder,
            Closed,
        };

        void requireRecording(const char *call) const;
        void requireRenderPass(const char *call) const;
        void endBlit();
        void bindConstants();

        MetalPlatform &m_platform;
        MTL::CommandBuffer *m_commands = nullptr;
        NS::SharedPtr<MTL::RenderCommandEncoder> m_render;
        NS::SharedPtr<MTL::BlitCommandEncoder> m_blit;
        std::array<std::byte, 128> m_constants{};
        bool m_pipeline_bound = false;
        MTL::PrimitiveType m_primitive = MTL::PrimitiveTypeTriangle;
        MTL::Buffer *m_index_buffer = nullptr;
        MTL::IndexType m_index_type = MTL::IndexTypeUInt16;
        std::uint64_t m_index_offset = 0;
        core::types::Vector2i m_target_size{};
        std::vector<LabelTarget> m_labels;
        std::vector<const MTL::Resource *> m_resources;
    };
} // namespace atmo::server::rendering::metal
