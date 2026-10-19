#include "canvas_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <type_traits>
#include <unordered_set>

#include "server/rendering/shader/builtin_shaders.hpp"
#include "spdlog/spdlog.h"

namespace atmo::server::rendering
{
    namespace
    {
        struct alignas(16) Constants {
            std::array<float, 2> target_size;
            std::uint32_t buffer_index;
            std::uint32_t first_element;
        };

        constexpr std::array<std::uint16_t, 6> quad_indices{ 0, 1, 2, 2, 1, 3 };
        constexpr std::size_t min_buffer_size = 64 * 1024;
        constexpr int max_parent_depth = 256;

        bool SameRegion(const Region2i &a, const Region2i &b) noexcept
        {
            return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
        }

        Region2i Intersect(const Region2i &a, const Region2i &b) noexcept
        {
            const int left = std::max(a.x, b.x);
            const int top = std::max(a.y, b.y);
            const int right = std::min(a.x + a.width, b.x + b.width);
            const int bottom = std::min(a.y + a.height, b.y + b.height);
            return { .x = left, .y = top, .width = std::max(right - left, 0), .height = std::max(bottom - top, 0) };
        }

        ColorRgba Multiply(const ColorRgba &a, const ColorRgba &b) noexcept
        {
            return { a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3] };
        }

        std::array<float, 4> Bounds(const Affine2d &quad) noexcept
        {
            const std::array<float, 4> xs{ quad.tx, quad.tx + quad.a, quad.tx + quad.c, quad.tx + quad.a + quad.c };
            const std::array<float, 4> ys{ quad.ty, quad.ty + quad.b, quad.ty + quad.d, quad.ty + quad.b + quad.d };
            return { *std::ranges::min_element(xs), *std::ranges::min_element(ys), *std::ranges::max_element(xs), *std::ranges::max_element(ys) };
        }

        Region2i PixelBounds(const Affine2d &to_pixels, const Rect2 &rect) noexcept
        {
            const Affine2d quad = to_pixels * Affine2d::Translation(rect.position) * Affine2d::Scaling(rect.size);
            const std::array<float, 4> bounds = Bounds(quad);
            const int left = static_cast<int>(std::floor(bounds[0]));
            const int top = static_cast<int>(std::floor(bounds[1]));
            return { .x = left, .y = top, .width = static_cast<int>(std::ceil(bounds[2])) - left, .height = static_cast<int>(std::ceil(bounds[3])) - top };
        }
    } // namespace

    CanvasRenderer::CanvasRenderer(RenderingPlatform &platform) : m_platform(platform) {}

    CanvasRenderer::~CanvasRenderer()
    {
        release();
    }

    bool CanvasRenderer::isAvailable() const noexcept
    {
        return !m_failed;
    }

    bool CanvasRenderer::ensureResources()
    {
        if (m_ready || m_failed)
            return m_ready;

        std::optional<ShaderBlob> blob = BuiltinShaderBlob(BuiltinShader::Canvas, m_platform.shaderTarget());
        if (!blob) {
            spdlog::warn("The {} platform has no built-in canvas shader", m_platform.name());
            m_failed = true;
            return false;
        }

        try {
            m_module = m_platform.shaderModuleCreate(*blob);

            m_white = m_platform.textureCreate({ .size = { 1, 1 }, .debug_name = "canvas white" });
            const std::array<std::byte, 4> white_pixel{ std::byte{ 0xFF }, std::byte{ 0xFF }, std::byte{ 0xFF }, std::byte{ 0xFF } };
            m_platform.textureUpload(m_white, { .width = 1, .height = 1 }, white_pixel);
            m_white_index = m_platform.bindlessIndex(m_white);

            for (bool repeat : { false, true })
                for (TextureFilter filter : { TextureFilter::Linear, TextureFilter::Nearest })
                    m_samplers[(filter == TextureFilter::Nearest ? 1 : 0) + (repeat ? 2 : 0)] = m_platform.samplerCreate(
                        {
                            .filter = filter == TextureFilter::Nearest ? Filter::Nearest : Filter::Linear,
                            .address = repeat ? AddressMode::Repeat : AddressMode::ClampToEdge,
                        });

            m_quad_indices = m_platform.bufferCreate(
                { .size = sizeof(quad_indices), .usage = BufferUsage::Index, .memory = MemoryType::Upload, .debug_name = "canvas quad indices" });
            m_platform.bufferWrite(m_quad_indices, 0, std::as_bytes(std::span(quad_indices)));
        } catch (const std::exception &e) {
            spdlog::error("Failed to create the canvas renderer resources: {}", e.what());
            release();
            m_failed = true;
            return false;
        }

        m_ready = true;
        return true;
    }

    void CanvasRenderer::release() noexcept
    {
        try {
            for (auto &[format, pipelines] : m_pipelines) {
                m_platform.destroy(pipelines.quads);
                m_platform.destroy(pipelines.mesh);
            }
            m_pipelines.clear();
            for (std::vector<GpuBuffer> *buffers : { &m_instance_buffers, &m_vertex_buffers }) {
                for (GpuBuffer &buffer : *buffers)
                    if (buffer.handle.isValid())
                        m_platform.destroy(buffer.handle);
                buffers->clear();
            }
            for (SamplerHandle &sampler : m_samplers) {
                if (sampler.isValid())
                    m_platform.destroy(sampler);
                sampler = {};
            }
            if (m_quad_indices.isValid())
                m_platform.destroy(m_quad_indices);
            if (m_white.isValid())
                m_platform.destroy(m_white);
            if (m_module.isValid())
                m_platform.destroy(m_module);
        } catch (const std::exception &e) {
            spdlog::error("Failed to release the canvas renderer resources: {}", e.what());
        }

        m_quad_indices = {};
        m_white = {};
        m_module = {};
        m_ready = false;
    }

    CanvasRenderer::Pipelines CanvasRenderer::pipelinesFor(TextureFormat format)
    {
        auto it = std::ranges::find(m_pipelines, format, &std::pair<TextureFormat, Pipelines>::first);
        if (it != m_pipelines.end())
            return it->second;

        Pipelines pipelines{
            .quads = m_platform.graphicsPipelineCreate(
                {
                    .vertex = { .module = m_module, .entry_point = "quadVertexMain" },
                    .fragment = { .module = m_module, .entry_point = "canvasFragmentMain" },
                    .color_format = format,
                    .blend = BlendMode::Alpha,
                    .debug_name = "canvas quads",
                }),
            .mesh = m_platform.graphicsPipelineCreate(
                {
                    .vertex = { .module = m_module, .entry_point = "meshVertexMain" },
                    .fragment = { .module = m_module, .entry_point = "canvasFragmentMain" },
                    .color_format = format,
                    .blend = BlendMode::Alpha,
                    .debug_name = "canvas mesh",
                }),
        };
        m_pipelines.emplace_back(format, pipelines);
        return pipelines;
    }

    SamplerHandle CanvasRenderer::samplerFor(TextureFilter filter, bool repeat) const
    {
        return m_samplers[(filter == TextureFilter::Nearest ? 1 : 0) + (repeat ? 2 : 0)];
    }

    void CanvasRenderer::beginFrame()
    {
        m_jobs.clear();
        m_instances.clear();
        m_vertices.clear();
        ++m_frame;
    }

    void CanvasRenderer::resolveGlobal(CanvasStorage &storage, CanvasItemData &item)
    {
        if (item.cache_frame == m_frame)
            return;
        item.cache_frame = m_frame;

        item.global = item.transform;
        item.global_visible = item.visible;
        item.global_modulate = item.modulate;

        CanvasItemRid parent_rid = item.parent;
        std::vector<CanvasItemData *> chain;
        for (int depth = 0; parent_rid.isValid() && depth < max_parent_depth; ++depth) {
            auto it = storage.items.find(parent_rid.raw());
            if (it == storage.items.end() || &it->second == &item)
                break;
            chain.push_back(&it->second);
            parent_rid = it->second.parent;
        }

        for (CanvasItemData *parent : chain) {
            item.global = parent->transform * item.global;
            item.global_visible = item.global_visible && parent->visible;
            item.global_modulate = Multiply(parent->modulate, item.global_modulate);
        }
    }

    void CanvasRenderer::sortCanvas(CanvasStorage &storage, CanvasData &canvas)
    {
        if (!canvas.sort_dirty)
            return;

        canvas.sorted = canvas.items;
        std::ranges::stable_sort(canvas.sorted, [&storage](CanvasItemRid lhs, CanvasItemRid rhs) {
            const CanvasItemData &a = storage.items.at(lhs.raw());
            const CanvasItemData &b = storage.items.at(rhs.raw());
            return a.z_index != b.z_index ? a.z_index < b.z_index : a.order < b.order;
        });
        canvas.sort_dirty = false;
    }

    void CanvasRenderer::pushQuad(
        Job &job, const Affine2d &transform, std::array<float, 4> uv, const ColorRgba &color, std::uint32_t texture, std::uint32_t sampler,
        const Region2i &scissor, FrameStats &stats)
    {
        const std::array<float, 4> bounds = Bounds(transform);
        if (scissor.width <= 0 || scissor.height <= 0 || bounds[2] <= static_cast<float>(scissor.x) || bounds[3] <= static_cast<float>(scissor.y) ||
            bounds[0] >= static_cast<float>(scissor.x + scissor.width) || bounds[1] >= static_cast<float>(scissor.y + scissor.height)) {
            ++stats.culled_items;
            return;
        }

        const auto index = static_cast<std::uint32_t>(m_instances.size());
        m_instances.push_back(
            {
                .basis = { transform.a, transform.b, transform.c, transform.d },
                .origin_uv_min = { transform.tx, transform.ty, uv[0], uv[1] },
                .uv_max = { uv[2], uv[3], 0.0f, 0.0f },
                .color = color,
                .resources = { texture, sampler, 0, 0 },
            });
        ++stats.instances;

        if (!job.batches.empty()) {
            Batch &last = job.batches.back();
            if (last.kind == BatchKind::Quads && last.first + last.count == index && SameRegion(last.scissor, scissor)) {
                ++last.count;
                return;
            }
        }
        job.batches.push_back({ .kind = BatchKind::Quads, .first = index, .count = 1, .scissor = scissor });
    }

    void CanvasRenderer::pushTriangles(Job &job, std::span<const std::array<float, 4>> positions_uv, std::span<const ColorRgba> colors, const Region2i &scissor)
    {
        if (positions_uv.empty() || scissor.width <= 0 || scissor.height <= 0)
            return;

        const auto first = static_cast<std::uint32_t>(m_vertices.size());
        const std::array<std::uint32_t, 4> resources{ m_white_index, m_platform.bindlessIndex(samplerFor(TextureFilter::Nearest, false)), 0, 0 };
        for (std::size_t i = 0; i < positions_uv.size(); ++i)
            m_vertices.push_back({ .position_uv = positions_uv[i], .color = colors[colors.size() == 1 ? 0 : i], .resources = resources });

        const auto count = static_cast<std::uint32_t>(positions_uv.size());
        if (!job.batches.empty()) {
            Batch &last = job.batches.back();
            if (last.kind == BatchKind::Mesh && last.first + last.count == first && SameRegion(last.scissor, scissor)) {
                last.count += count;
                return;
            }
        }
        job.batches.push_back({ .kind = BatchKind::Mesh, .first = first, .count = count, .scissor = scissor });
    }

    std::size_t CanvasRenderer::prepare(
        CanvasStorage &storage, CanvasRid canvas_rid, const Affine2d &view, core::types::Vector2i target_size, FrameStats &stats,
        const std::optional<Region2i> &content, const ColorRgba &content_color)
    {
        m_jobs.push_back({ .target_size = target_size });
        const std::size_t job_index = m_jobs.size() - 1;

        if (target_size.x <= 0 || target_size.y <= 0 || !ensureResources())
            return job_index;

        Job &job = m_jobs[job_index];

        Region2i full{ .width = target_size.x, .height = target_size.y };
        if (content) {
            full = Intersect(full, *content);
            const Affine2d fill = Affine2d::Translation({ static_cast<float>(full.x), static_cast<float>(full.y) }) *
                Affine2d::Scaling({ static_cast<float>(full.width), static_cast<float>(full.height) });
            pushQuad(
                job,
                fill,
                { 0.0f, 0.0f, 1.0f, 1.0f },
                content_color,
                m_white_index,
                m_platform.bindlessIndex(samplerFor(TextureFilter::Nearest, false)),
                full,
                stats);
        }

        auto canvas_it = storage.canvases.find(canvas_rid.raw());
        if (canvas_it == storage.canvases.end()) {
            stats.draw_calls += static_cast<std::uint32_t>(job.batches.size());
            return job_index;
        }
        CanvasData &canvas = canvas_it->second;
        sortCanvas(storage, canvas);
        std::vector<Region2i> clips;
        std::unordered_set<std::uint32_t> sampled;
        std::vector<std::array<float, 4>> positions;
        std::vector<ColorRgba> colors;

        for (CanvasItemRid rid : canvas.sorted) {
            auto item_it = storage.items.find(rid.raw());
            if (item_it == storage.items.end())
                continue;
            CanvasItemData &item = item_it->second;
            resolveGlobal(storage, item);
            if (!item.global_visible || item.commands.empty())
                continue;
            ++stats.canvas_items;

            const Affine2d to_pixels = view * item.global;
            clips.assign(1, full);

            for (const CanvasCommand &command : item.commands) {
                std::visit(
                    [&](const auto &cmd) {
                        using T = std::decay_t<decltype(cmd)>;
                        if constexpr (std::is_same_v<T, CanvasRectCommand>) {
                            pushQuad(
                                job,
                                to_pixels * Affine2d::Translation(cmd.rect.position) * Affine2d::Scaling(cmd.rect.size),
                                { 0.0f, 0.0f, 1.0f, 1.0f },
                                Multiply(cmd.color, item.global_modulate),
                                m_white_index,
                                m_platform.bindlessIndex(samplerFor(TextureFilter::Nearest, false)),
                                clips.back(),
                                stats);
                        } else if constexpr (std::is_same_v<T, CanvasTextureRectCommand>) {
                            auto texture_it = storage.textures.find(cmd.texture.raw());
                            if (texture_it == storage.textures.end() || !texture_it->second.handle.isValid() || texture_it->second.size.x <= 0 ||
                                texture_it->second.size.y <= 0)
                                return;
                            const TextureEntry &texture = texture_it->second;
                            const float width = static_cast<float>(texture.size.x);
                            const float height = static_cast<float>(texture.size.y);

                            std::array<float, 4> uv{ 0.0f, 0.0f, 1.0f, 1.0f };
                            if (cmd.options.source) {
                                const Rect2 &source = *cmd.options.source;
                                uv = { source.position.x / width,
                                       source.position.y / height,
                                       (source.position.x + source.size.x) / width,
                                       (source.position.y + source.size.y) / height };
                            } else if (cmd.options.repeat) {
                                uv = { 0.0f, 0.0f, cmd.rect.size.x / width, cmd.rect.size.y / height };
                            }

                            if (sampled.insert(texture.handle.value).second)
                                job.sampled.push_back(texture.handle);
                            pushQuad(
                                job,
                                to_pixels * Affine2d::Translation(cmd.rect.position) * Affine2d::Scaling(cmd.rect.size),
                                uv,
                                Multiply(cmd.options.modulate, item.global_modulate),
                                m_platform.bindlessIndex(texture.handle),
                                m_platform.bindlessIndex(samplerFor(cmd.options.filter, cmd.options.repeat)),
                                clips.back(),
                                stats);
                        } else if constexpr (std::is_same_v<T, CanvasPolygonCommand>) {
                            if (cmd.points.size() < 3)
                                return;
                            positions.clear();
                            colors.clear();
                            for (std::size_t i = 1; i + 1 < cmd.points.size(); ++i) {
                                for (std::size_t index : { std::size_t{ 0 }, i, i + 1 }) {
                                    const core::types::Vector2 point = to_pixels.apply(cmd.points[index]);
                                    positions.push_back({ point.x, point.y, 0.0f, 0.0f });
                                    const ColorRgba &color = cmd.colors.empty() ? white : cmd.colors[cmd.colors.size() == cmd.points.size() ? index : 0];
                                    colors.push_back(Multiply(color, item.global_modulate));
                                }
                            }
                            pushTriangles(job, positions, colors, clips.back());
                            stats.vertices += static_cast<std::uint32_t>(positions.size());
                        } else if constexpr (std::is_same_v<T, CanvasPolylineCommand>) {
                            if (cmd.points.size() < 2)
                                return;
                            positions.clear();
                            const std::size_t segments = cmd.closed ? cmd.points.size() : cmd.points.size() - 1;
                            const float half = cmd.width * 0.5f;
                            for (std::size_t s = 0; s < segments; ++s) {
                                const core::types::Vector2 &from = cmd.points[s];
                                const core::types::Vector2 &to = cmd.points[(s + 1) % cmd.points.size()];
                                const float dx = to.x - from.x;
                                const float dy = to.y - from.y;
                                const float length = std::sqrt((dx * dx) + (dy * dy));
                                if (length <= 0.0f)
                                    continue;
                                const float nx = -dy / length * half;
                                const float ny = dx / length * half;
                                const std::array corners{
                                    to_pixels.apply({ from.x + nx, from.y + ny }),
                                    to_pixels.apply({ from.x - nx, from.y - ny }),
                                    to_pixels.apply({ to.x + nx, to.y + ny }),
                                    to_pixels.apply({ to.x - nx, to.y - ny }),
                                };
                                for (std::size_t index : { 0, 1, 2, 2, 1, 3 }) positions.push_back({ corners[index].x, corners[index].y, 0.0f, 0.0f });
                            }
                            const ColorRgba color = Multiply(cmd.color, item.global_modulate);
                            pushTriangles(job, positions, { &color, 1 }, clips.back());
                            stats.vertices += static_cast<std::uint32_t>(positions.size());
                        } else if constexpr (std::is_same_v<T, CanvasClipPushCommand>) {
                            clips.push_back(Intersect(clips.back(), PixelBounds(to_pixels, cmd.rect)));
                        } else if constexpr (std::is_same_v<T, CanvasClipPopCommand>) {
                            if (clips.size() > 1)
                                clips.pop_back();
                        }
                    },
                    command);
            }
        }

        stats.draw_calls += static_cast<std::uint32_t>(job.batches.size());
        return job_index;
    }

    void CanvasRenderer::ensureCapacity(GpuBuffer &buffer, std::size_t bytes, std::string_view name)
    {
        if (buffer.handle.isValid() && buffer.capacity >= bytes)
            return;

        if (buffer.handle.isValid())
            m_platform.destroy(buffer.handle);

        buffer.capacity = std::max({ bytes, buffer.capacity * 2, min_buffer_size });
        buffer.handle = m_platform.bufferCreate({ .size = buffer.capacity, .usage = BufferUsage::Storage, .memory = MemoryType::Upload, .debug_name = name });
    }

    void CanvasRenderer::upload()
    {
        if (!m_ready)
            return;

        const std::size_t frames = std::max<std::uint32_t>(m_platform.caps().frames_in_flight, 1);
        m_instance_buffers.resize(frames);
        m_vertex_buffers.resize(frames);
        const std::size_t slot = m_frame % frames;

        if (!m_instances.empty()) {
            ensureCapacity(m_instance_buffers[slot], m_instances.size() * sizeof(Instance), "canvas instances");
            m_platform.bufferWrite(m_instance_buffers[slot].handle, 0, std::as_bytes(std::span(m_instances)));
        }
        if (!m_vertices.empty()) {
            ensureCapacity(m_vertex_buffers[slot], m_vertices.size() * sizeof(Vertex), "canvas vertices");
            m_platform.bufferWrite(m_vertex_buffers[slot].handle, 0, std::as_bytes(std::span(m_vertices)));
        }
    }

    void CanvasRenderer::record(std::size_t job_index, CommandList &list, TextureFormat format)
    {
        const Job &job = m_jobs.at(job_index);
        if (job.batches.empty() || !m_ready)
            return;

        const Pipelines pipelines = pipelinesFor(format);
        const std::size_t slot = m_frame % m_instance_buffers.size();
        const auto width = static_cast<float>(job.target_size.x);
        const auto height = static_cast<float>(job.target_size.y);

        list.viewportSet({ .width = width, .height = height });

        std::optional<BatchKind> bound;
        for (const Batch &batch : job.batches) {
            if (bound != batch.kind) {
                list.pipelineBind(batch.kind == BatchKind::Quads ? pipelines.quads : pipelines.mesh);
                if (batch.kind == BatchKind::Quads)
                    list.indexBufferBind(m_quad_indices, IndexType::Uint16, 0);
                bound = batch.kind;
            }

            const BufferHandle buffer = batch.kind == BatchKind::Quads ? m_instance_buffers[slot].handle : m_vertex_buffers[slot].handle;
            const Constants constants{
                .target_size = { width, height },
                .buffer_index = m_platform.bindlessIndex(buffer),
                .first_element = batch.first,
            };
            list.scissorSet(batch.scissor);
            list.constantsPush(std::as_bytes(std::span(&constants, 1)));

            if (batch.kind == BatchKind::Quads)
                list.drawIndexed(static_cast<std::uint32_t>(quad_indices.size()), batch.count, 0, 0, 0);
            else
                list.draw(batch.count, 1, 0, 0);
        }
    }

    const std::vector<TextureHandle> &CanvasRenderer::sampledTextures(std::size_t job) const
    {
        return m_jobs.at(job).sampled;
    }

    const std::vector<CanvasRenderer::Batch> &CanvasRenderer::batches(std::size_t job) const
    {
        return m_jobs.at(job).batches;
    }
} // namespace atmo::server::rendering
