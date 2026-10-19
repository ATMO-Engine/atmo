#include "render_scene.hpp"

#include <cmath>
#include <exception>
#include <stdexcept>

#include "spdlog/spdlog.h"

namespace atmo::server::rendering
{
    namespace
    {
        constexpr TextureUsage texture_usage = TextureUsage::Sampled | TextureUsage::CopyDst | TextureUsage::CopySrc;
        constexpr TextureUsage target_usage = TextureUsage::RenderTarget | TextureUsage::Sampled | TextureUsage::CopySrc;
    } // namespace

    RenderScene::RenderScene(RenderingPlatform &platform) : m_platform(platform), m_canvas(platform) {}

    RenderScene::~RenderScene()
    {
        for (auto &[raw, window] : m_windows) {
            try {
                m_platform.swapchainDestroy(window.swapchain);
            } catch (const std::exception &e) {
                spdlog::error("Failed to destroy a swapchain: {}", e.what());
            }
        }
        m_windows.clear();

        for (auto &[raw, entry] : m_storage.textures) destroyTexture(entry);
        m_storage.textures.clear();
    }

    void RenderScene::destroyTexture(TextureEntry &entry) noexcept
    {
        if (!entry.handle.isValid())
            return;
        try {
            m_platform.destroy(entry.handle);
        } catch (const std::exception &e) {
            spdlog::error("Failed to destroy a texture: {}", e.what());
        }
        entry.handle = {};
    }

    CanvasItemData *RenderScene::item(CanvasItemRid rid)
    {
        auto it = m_storage.items.find(rid.raw());
        return it == m_storage.items.end() ? nullptr : &it->second;
    }

    RenderScene::Viewport *RenderScene::viewport(ViewportRid rid)
    {
        auto it = m_viewports.find(rid.raw());
        return it == m_viewports.end() ? nullptr : &it->second;
    }

    void RenderScene::textureCreate(TextureRid rid, const Texture2dDesc &desc, std::span<const std::byte> pixels)
    {
        TextureEntry entry{ .size = desc.size };
        entry.handle =
            m_platform.textureCreate({ .size = desc.size, .format = TextureFormat::Rgba8Unorm, .usage = texture_usage, .debug_name = desc.debug_name });
        if (!pixels.empty()) {
            try {
                m_platform.textureUpload(entry.handle, { .width = desc.size.x, .height = desc.size.y }, pixels);
            } catch (...) {
                destroyTexture(entry);
                throw;
            }
        }
        m_storage.textures[rid.raw()] = entry;
    }

    void RenderScene::textureUpdate(TextureRid rid, const Region2i &region, std::span<const std::byte> pixels)
    {
        auto it = m_storage.textures.find(rid.raw());
        if (it == m_storage.textures.end())
            throw std::invalid_argument("texture2dUpdate: unknown texture");
        m_platform.textureUpload(it->second.handle, region, pixels);
    }

    void RenderScene::textureFree(TextureRid rid)
    {
        auto it = m_storage.textures.find(rid.raw());
        if (it == m_storage.textures.end())
            return;
        destroyTexture(it->second);
        m_storage.textures.erase(it);
    }

    void RenderScene::textureRequestData(TextureRid rid, std::uint64_t request)
    {
        m_readbacks.emplace_back(request, rid);
    }

    void RenderScene::viewportCreate(ViewportRid rid, TextureRid texture)
    {
        m_viewports[rid.raw()] = Viewport{ .texture = texture };
        if (texture.isValid())
            m_storage.textures[texture.raw()] = {};
    }

    void RenderScene::resizeViewportTarget(Viewport &target)
    {
        TextureEntry &entry = m_storage.textures[target.texture.raw()];
        destroyTexture(entry);
        entry.size = target.size;
        if (target.size.x > 0 && target.size.y > 0)
            entry.handle =
                m_platform.textureCreate({ .size = target.size, .format = TextureFormat::Rgba8Unorm, .usage = target_usage, .debug_name = "viewport" });
    }

    void RenderScene::viewportSetSize(ViewportRid rid, core::types::Vector2i size)
    {
        Viewport *target = viewport(rid);
        if (!target)
            return;
        if (target->size.x == size.x && target->size.y == size.y)
            return;
        target->size = size;
        if (target->texture.isValid())
            resizeViewportTarget(*target);
    }

    void RenderScene::viewportSetCanvas(ViewportRid rid, CanvasRid canvas)
    {
        if (Viewport *target = viewport(rid))
            target->canvas = canvas;
    }

    void RenderScene::viewportSetCanvasTransform(ViewportRid rid, const Affine2d &transform)
    {
        if (Viewport *target = viewport(rid))
            target->transform = transform;
    }

    void RenderScene::viewportSetClearColor(ViewportRid rid, const ColorRgba &color)
    {
        if (Viewport *target = viewport(rid))
            target->clear = color;
    }

    void RenderScene::viewportSetActive(ViewportRid rid, bool active)
    {
        if (Viewport *target = viewport(rid))
            target->active = active;
    }

    void RenderScene::viewportSetContentRect(ViewportRid rid, const std::optional<Rect2> &rect)
    {
        if (Viewport *target = viewport(rid))
            target->content = rect;
    }

    void RenderScene::viewportSetBorderColor(ViewportRid rid, const ColorRgba &color)
    {
        if (Viewport *target = viewport(rid))
            target->border = color;
    }

    void RenderScene::addPass(
        std::string name, const Viewport &target, TextureHandle attachment, core::types::Vector2i size, const Affine2d &to_pixels, TextureFormat format,
        FrameStats &stats)
    {
        std::optional<Region2i> content;
        if (target.content) {
            const core::types::Vector2 min = to_pixels.apply(target.content->position);
            const core::types::Vector2 max =
                to_pixels.apply({ target.content->position.x + target.content->size.x, target.content->position.y + target.content->size.y });
            const int left = static_cast<int>(std::lround(min.x));
            const int top = static_cast<int>(std::lround(min.y));
            content =
                Region2i{ .x = left, .y = top, .width = static_cast<int>(std::lround(max.x)) - left, .height = static_cast<int>(std::lround(max.y)) - top };
        }

        m_passes.push_back(
            {
                .name = std::move(name),
                .attachment = { .texture = attachment, .load = LoadOp::Clear, .store = StoreOp::Store, .clear_color = content ? target.border : target.clear },
                .job = m_canvas.prepare(m_storage, target.canvas, to_pixels * target.transform, size, stats, content, target.clear),
                .format = format,
            });
    }

    void RenderScene::viewportFree(ViewportRid rid)
    {
        auto it = m_viewports.find(rid.raw());
        if (it == m_viewports.end())
            return;
        if (it->second.texture.isValid())
            textureFree(it->second.texture);
        m_viewports.erase(it);
    }

    void
    RenderScene::windowAttach(WindowRid rid, ViewportRid viewport, const NativeWindow &window, core::types::Vector2i pixel_size, core::types::Vector2 dpi_scale)
    {
        SwapchainHandle swapchain = m_platform.swapchainCreate(window, { .size = pixel_size });
        if (!swapchain.isValid())
            return;
        m_windows[rid.raw()] = { .swapchain = swapchain, .viewport = viewport, .dpi_scale = dpi_scale };
    }

    void RenderScene::windowResize(WindowRid rid, core::types::Vector2i pixel_size, core::types::Vector2 dpi_scale)
    {
        auto it = m_windows.find(rid.raw());
        if (it == m_windows.end())
            return;
        it->second.dpi_scale = dpi_scale;
        m_platform.swapchainResize(it->second.swapchain, pixel_size);
    }

    void RenderScene::windowDetach(WindowRid rid)
    {
        auto it = m_windows.find(rid.raw());
        if (it == m_windows.end())
            return;
        m_platform.swapchainDestroy(it->second.swapchain);
        m_windows.erase(it);
    }

    void RenderScene::canvasCreate(CanvasRid rid)
    {
        m_storage.canvases[rid.raw()] = {};
    }

    void RenderScene::canvasFree(CanvasRid rid)
    {
        auto it = m_storage.canvases.find(rid.raw());
        if (it == m_storage.canvases.end())
            return;
        for (CanvasItemRid child : it->second.items)
            if (CanvasItemData *data = item(child))
                data->canvas = {};
        m_storage.canvases.erase(it);
    }

    void RenderScene::canvasItemCreate(CanvasItemRid rid, CanvasRid canvas)
    {
        CanvasItemData data{ .canvas = canvas, .order = m_storage.next_order++ };
        auto canvas_it = m_storage.canvases.find(canvas.raw());
        if (canvas_it != m_storage.canvases.end()) {
            data.slot = canvas_it->second.items.size();
            canvas_it->second.items.push_back(rid);
            canvas_it->second.sort_dirty = true;
        } else {
            data.canvas = {};
        }
        m_storage.items[rid.raw()] = std::move(data);
    }

    void RenderScene::canvasItemSetParent(CanvasItemRid rid, CanvasItemRid parent)
    {
        CanvasItemData *data = item(rid);
        if (!data)
            return;
        for (CanvasItemRid ancestor = parent; ancestor.isValid();) {
            if (ancestor == rid)
                throw std::invalid_argument("canvasItemSetParent: an item cannot be its own ancestor");
            CanvasItemData *next = item(ancestor);
            ancestor = next ? next->parent : CanvasItemRid{};
        }
        data->parent = parent;
    }

    void RenderScene::canvasItemSetTransform(CanvasItemRid rid, const Affine2d &transform)
    {
        if (CanvasItemData *data = item(rid))
            data->transform = transform;
    }

    void RenderScene::canvasItemSetZIndex(CanvasItemRid rid, int z_index)
    {
        CanvasItemData *data = item(rid);
        if (!data || data->z_index == z_index)
            return;
        data->z_index = z_index;
        auto canvas_it = m_storage.canvases.find(data->canvas.raw());
        if (canvas_it != m_storage.canvases.end())
            canvas_it->second.sort_dirty = true;
    }

    void RenderScene::canvasItemSetVisible(CanvasItemRid rid, bool visible)
    {
        if (CanvasItemData *data = item(rid))
            data->visible = visible;
    }

    void RenderScene::canvasItemSetModulate(CanvasItemRid rid, const ColorRgba &color)
    {
        if (CanvasItemData *data = item(rid))
            data->modulate = color;
    }

    void RenderScene::canvasItemClear(CanvasItemRid rid)
    {
        if (CanvasItemData *data = item(rid))
            data->commands.clear();
    }

    void RenderScene::canvasItemAddCommand(CanvasItemRid rid, CanvasCommand command)
    {
        if (CanvasItemData *data = item(rid))
            data->commands.push_back(std::move(command));
    }

    void RenderScene::canvasItemFree(CanvasItemRid rid)
    {
        auto it = m_storage.items.find(rid.raw());
        if (it == m_storage.items.end())
            return;

        auto canvas_it = m_storage.canvases.find(it->second.canvas.raw());
        if (canvas_it != m_storage.canvases.end()) {
            std::vector<CanvasItemRid> &items = canvas_it->second.items;
            const std::size_t slot = it->second.slot;
            if (slot < items.size() && items[slot] == rid) {
                items[slot] = items.back();
                if (CanvasItemData *moved = item(items[slot]))
                    moved->slot = slot;
                items.pop_back();
            }
            canvas_it->second.sort_dirty = true;
        }
        m_storage.items.erase(it);
    }

    void RenderScene::render(FrameStats &stats)
    {
        m_canvas.beginFrame();
        m_graph.reset();
        m_passes.clear();
        m_acquired.clear();

        for (auto &[raw, target] : m_viewports) {
            if (!target.active || !target.texture.isValid())
                continue;
            const TextureEntry &entry = m_storage.textures[target.texture.raw()];
            if (!entry.handle.isValid())
                continue;
            addPass("Viewport", target, entry.handle, target.size, {}, TextureFormat::Rgba8Unorm, stats);
        }

        for (auto &[raw, window] : m_windows) {
            const Viewport *target = viewport(window.viewport);
            if (!target || !target->active)
                continue;
            const TextureHandle image = m_platform.swapchainAcquire(window.swapchain);
            if (!image.isValid())
                continue;
            m_acquired.push_back(window.swapchain);
            m_graph.addPresent(image);

            addPass(
                "Window",
                *target,
                image,
                m_platform.textureSize(image),
                Affine2d::Scaling(window.dpi_scale),
                m_platform.swapchainFormat(window.swapchain),
                stats);
        }

        if (m_passes.empty())
            return;

        m_canvas.upload();
        CommandList &list = m_platform.commandListAcquire(QueueType::Graphics);
        for (const Pass &pass : m_passes) {
            std::vector<TextureHandle> sampled = m_canvas.sampledTextures(pass.job);
            std::erase(sampled, pass.attachment.texture);
            m_graph.addPass(
                { .name = pass.name, .color_attachments = { pass.attachment }, .sampled = std::move(sampled) },
                [this, job = pass.job, format = pass.format](CommandList &commands) { m_canvas.record(job, commands, format); });
        }
        m_graph.execute(list);

        CommandList *lists[] = { &list };
        m_platform.submit(lists);
        for (SwapchainHandle swapchain : m_acquired) m_platform.swapchainPresent(swapchain);
    }

    std::vector<std::pair<std::uint64_t, TextureData>> RenderScene::processReadbacks()
    {
        std::vector<std::pair<std::uint64_t, TextureData>> results;
        for (auto &[request, rid] : m_readbacks) {
            TextureData data;
            auto it = m_storage.textures.find(rid.raw());
            if (it != m_storage.textures.end() && it->second.handle.isValid()) {
                try {
                    data.size = it->second.size;
                    data.pixels = m_platform.textureReadback(it->second.handle);
                } catch (const std::exception &e) {
                    spdlog::error("Texture readback failed: {}", e.what());
                    data = {};
                }
            }
            results.emplace_back(request, std::move(data));
        }
        m_readbacks.clear();
        return results;
    }

    std::size_t RenderScene::windowCount() const noexcept
    {
        return m_windows.size();
    }

    std::size_t RenderScene::canvasItemCount() const noexcept
    {
        return m_storage.items.size();
    }

    const CanvasRenderer &RenderScene::canvasRenderer() const noexcept
    {
        return m_canvas;
    }
} // namespace atmo::server::rendering
