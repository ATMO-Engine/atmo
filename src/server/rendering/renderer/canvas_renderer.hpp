#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "server/rendering/platform/rendering_platform.hpp"
#include "server/rendering/renderer/canvas.hpp"

namespace atmo::server::rendering
{
    class CanvasRenderer
    {
    public:
        struct alignas(16) Instance {
            std::array<float, 4> basis;
            std::array<float, 4> origin_uv_min;
            std::array<float, 4> uv_max;
            ColorRgba color;
            std::array<std::uint32_t, 4> resources;
            std::array<float, 4> corner_radii;
            std::array<float, 4> border_widths;
            ColorRgba border_color;
        };

        struct alignas(16) Vertex {
            std::array<float, 4> position_uv;
            ColorRgba color;
            std::array<std::uint32_t, 4> resources;
        };

        enum class BatchKind : std::uint8_t {
            Quads,
            Mesh,
        };

        struct Batch {
            BatchKind kind = BatchKind::Quads;
            std::uint32_t first = 0;
            std::uint32_t count = 0;
            Region2i scissor;
        };

        explicit CanvasRenderer(RenderingPlatform &platform);
        ~CanvasRenderer();

        CanvasRenderer(const CanvasRenderer &) = delete;
        CanvasRenderer &operator=(const CanvasRenderer &) = delete;

        [[nodiscard]] bool isAvailable() const noexcept;

        void beginFrame();
        std::size_t beginJob(
            core::types::Vector2i target_size, FrameStats &stats, const std::optional<Region2i> &content = std::nullopt,
            const ColorRgba &content_color = white);
        void addCanvas(std::size_t job, CanvasStorage &storage, CanvasRid canvas, const Affine2d &view, bool clip_to_content, FrameStats &stats);
        void endJob(std::size_t job, FrameStats &stats);
        void upload();
        void record(std::size_t job, CommandList &list, TextureFormat format);

        [[nodiscard]] const std::vector<TextureHandle> &sampledTextures(std::size_t job) const;
        [[nodiscard]] const std::vector<Batch> &batches(std::size_t job) const;

    private:
        struct Job {
            core::types::Vector2i target_size{};
            Region2i full;
            Region2i content;
            bool ready = false;
            std::vector<Batch> batches;
            std::vector<TextureHandle> sampled;
        };

        struct Pipelines {
            PipelineHandle quads;
            PipelineHandle mesh;
        };

        struct GpuBuffer {
            BufferHandle handle;
            std::size_t capacity = 0;
        };

        bool ensureResources();
        void release() noexcept;
        Pipelines pipelinesFor(TextureFormat format);
        SamplerHandle samplerFor(TextureFilter filter, bool repeat) const;
        void resolveGlobal(CanvasStorage &storage, CanvasItemData &item);
        void sortCanvas(CanvasStorage &storage, CanvasData &canvas);
        void ensureCapacity(GpuBuffer &buffer, std::size_t bytes, std::string_view name);

        void pushQuad(
            Job &job, const Affine2d &transform, std::array<float, 4> uv, const ColorRgba &color, std::uint32_t texture, std::uint32_t sampler,
            const Region2i &scissor, FrameStats &stats, const RectStyle *style = nullptr, core::types::Vector2 size = {});
        void pushTriangles(Job &job, std::span<const std::array<float, 4>> positions_uv, std::span<const ColorRgba> colors, const Region2i &scissor);

        RenderingPlatform &m_platform;
        ShaderModuleHandle m_module;
        TextureHandle m_white;
        BufferHandle m_quad_indices;
        std::array<SamplerHandle, 4> m_samplers{};
        std::vector<std::pair<TextureFormat, Pipelines>> m_pipelines;
        std::vector<GpuBuffer> m_instance_buffers;
        std::vector<GpuBuffer> m_vertex_buffers;

        std::vector<Job> m_jobs;
        std::vector<Region2i> m_clips;
        std::unordered_set<std::uint32_t> m_sampled;
        std::vector<Instance> m_instances;
        std::vector<Vertex> m_vertices;
        std::uint64_t m_frame = 0;
        std::uint32_t m_white_index = 0;
        bool m_ready = false;
        bool m_failed = false;
    };
} // namespace atmo::server::rendering
