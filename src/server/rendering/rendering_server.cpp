#include "rendering_server.hpp"

#include <cassert>
#include <chrono>
#include <cstring>
#include <stdexcept>

#include "SDL3/SDL_video.h"
#include "core/event/event_registry.hpp"
#include "core/event/events/sdl_event/input_event/input_event.hpp"
#include "impl/profiler.hpp"
#include "server/rendering/renderer/render_scene.hpp"
#include "spdlog/spdlog.h"

namespace atmo::server::rendering
{
#if defined(__APPLE__)
    static constexpr SDL_WindowFlags legacy_renderer_window_flags = SDL_WINDOW_METAL;
#else
    static constexpr SDL_WindowFlags legacy_renderer_window_flags = SDL_WINDOW_VULKAN;
#endif

    namespace
    {
        ColorRgba ToRgba(const core::types::Color &color) noexcept
        {
            return { color.r, color.g, color.b, color.a };
        }

        bool UsesMetalView(const RenderingPlatform &platform)
        {
#if defined(__APPLE__)
            const char *driver = SDL_GetCurrentVideoDriver();
            return platform.caps().presentation && driver && std::strcmp(driver, "cocoa") == 0;
#else
            return false;
#endif
        }
    } // namespace

    RenderingServer &RenderingServer::Instance()
    {
        static RenderingServer instance;
        return instance;
    }

    RenderingServer::RenderingServer() = default;

    RenderingServer::~RenderingServer()
    {
        shutdown();
    }

    bool RenderingServer::init(const RenderingSettings &settings)
    {
        if (m_initialized) {
            spdlog::warn("RenderingServer is already initialized");
            return true;
        }

        m_platform = CreatePlatform(settings);
        if (!m_platform) {
            spdlog::error("No rendering platform could be initialized");
            return false;
        }

        m_main_thread = std::this_thread::get_id();
        m_legacy_renderer = settings.legacy_sdl_renderer && !m_platform->caps().presentation;
        m_scene = std::make_unique<RenderScene>(*m_platform);
        m_commands.setMode(settings.inline_commands ? CommandQueue::Mode::Inline : CommandQueue::Mode::Deferred);
        m_submitted_frames = 0;
        m_completed_frames = 0;
        m_stats = {};
        m_render_stats = {};
        m_initialized = true;

        if (settings.render_thread && !settings.inline_commands)
            m_render_thread.start([this](CommandQueue::Batch &batch) { renderFrame(&batch); });

        spdlog::info(
            "RenderingServer initialized with the {} platform ({}){}",
            m_platform->name(),
            m_platform->caps().device_name,
            m_render_thread.running() ? ", render thread on" : "");
        return true;
    }

    void RenderingServer::shutdown()
    {
        if (!m_initialized)
            return;
        assertMainThread();

        if (m_render_thread.running()) {
            m_render_thread.wait();
            m_render_thread.submit(m_commands.take());
            m_render_thread.stop();
        } else {
            m_commands.flush();
        }

        m_scene.reset();
        m_windows.destroyAll();

        m_window_rids = {};
        m_viewport_rids = {};
        m_texture_rids = {};
        m_canvas_rids = {};
        m_canvas_item_rids = {};
        m_window_viewports.clear();
        m_texture_sizes.clear();
        m_viewport_textures.clear();
        m_viewport_sizes.clear();
        m_readback_callbacks.clear();
        {
            std::scoped_lock lock(m_completion_mutex);
            m_completed_readbacks.clear();
        }

        m_platform->shutdown();
        m_platform.reset();
        m_initialized = false;
    }

    bool RenderingServer::isInitialized() const noexcept
    {
        return m_initialized;
    }

    PlatformKind RenderingServer::platformKind() const
    {
        if (!m_platform)
            throw std::logic_error("RenderingServer is not initialized");
        return m_platform->kind();
    }

    bool RenderingServer::renderThreadActive() const noexcept
    {
        return m_render_thread.running();
    }

    void RenderingServer::pollEvents()
    {
        if (!m_initialized)
            return;
        assertMainThread();

        m_windows.pollEvents(
            [this](WindowRid window, const WindowEvent &event) {
                if (event.type != WindowEventType::Resized && event.type != WindowEventType::DisplayScaleChanged)
                    return;

                const core::types::Vector2i pixel_size = m_windows.pixelSize(window);
                const core::types::Vector2 dpi_scale = m_windows.dpiScale(window);
                enqueue([this, window, pixel_size, dpi_scale] { m_scene->windowResize(window, pixel_size, dpi_scale); });
            },
            [](const SDL_Event &event) {
                auto input_event = core::event::EventRegistry::Create<core::event::events::InputEvent>("Event::SDLEvent::InputEvent");
                input_event->sdl_event = event;
                core::event::EventRegistry::Dispatch(input_event);
            });
    }

    void RenderingServer::renderFrame(CommandQueue::Batch *batch)
    {
        ATMO_PROFILE_SCOPE_N("RenderingServer::renderFrame");
        const auto start = std::chrono::steady_clock::now();
        FrameStats stats;
        std::vector<std::pair<std::uint64_t, TextureData>> readbacks;

        try {
            m_platform->frameBegin();
            try {
                if (batch)
                    batch->execute();
                else
                    m_commands.flush();
                m_scene->render(stats);
            } catch (const std::exception &e) {
                spdlog::error("Rendering the frame failed: {}", e.what());
            }
            m_platform->frameEnd();
            readbacks = m_scene->processReadbacks();
        } catch (const std::exception &e) {
            spdlog::error("The rendering platform failed to finish the frame: {}", e.what());
        }

        stats.render_cpu_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

        std::scoped_lock lock(m_completion_mutex);
        stats.frame = m_render_stats.frame + 1;
        m_render_stats = stats;
        for (auto &readback : readbacks) m_completed_readbacks.push_back(std::move(readback));
    }

    void RenderingServer::completeFrames()
    {
        std::vector<std::pair<std::uint64_t, TextureData>> readbacks;
        {
            std::scoped_lock lock(m_completion_mutex);
            readbacks.swap(m_completed_readbacks);
            m_stats = m_render_stats;
        }

        m_completed_frames = m_submitted_frames;
        m_windows.destroyPending(m_completed_frames);

        for (auto &[request, data] : readbacks) {
            auto it = m_readback_callbacks.find(request);
            if (it == m_readback_callbacks.end())
                continue;
            std::function<void(TextureData)> callback = std::move(it->second);
            m_readback_callbacks.erase(it);
            if (callback)
                callback(std::move(data));
        }
    }

    void RenderingServer::sync()
    {
        if (!m_initialized)
            return;
        assertMainThread();
        ATMO_PROFILE_SCOPE_N("RenderingServer::sync");

        if (m_render_thread.running()) {
            m_render_thread.wait();
            completeFrames();
            ++m_submitted_frames;
            m_render_thread.submit(m_commands.take());
        } else {
            ++m_submitted_frames;
            renderFrame(nullptr);
            completeFrames();
        }
    }

    void RenderingServer::waitIdle()
    {
        if (!m_initialized)
            return;
        assertMainThread();
        m_render_thread.wait();
        completeFrames();
    }

    template <RidKind K> bool RenderingServer::alive(const RidAllocator<K> &allocator, Rid<K> rid, std::string_view call) const
    {
        if (!m_initialized)
            return false;
        assertMainThread();
        if (allocator.isAlive(rid))
            return true;
        if (rid.isValid())
            spdlog::warn("{}: RID {:#x} is not alive", call, rid.raw());
        return false;
    }

    WindowRid RenderingServer::windowCreate(const WindowDesc &desc)
    {
        if (!m_initialized) {
            spdlog::error("windowCreate called before RenderingServer::init");
            return {};
        }
        assertMainThread();

        WindowRid window = m_window_rids.allocate();

        SDL_WindowFlags flags = m_platform->windowFlags();
        if (m_legacy_renderer)
            flags |= legacy_renderer_window_flags;

        if (!m_windows.create(window, desc, flags, UsesMetalView(*m_platform))) {
            m_window_rids.release(window);
            return {};
        }

        const ViewportRid viewport = m_viewport_rids.allocate();
        m_window_viewports[window.raw()] = viewport;

        const NativeWindow native = m_windows.nativeWindow(window);
        const core::types::Vector2i pixel_size = m_windows.pixelSize(window);
        const core::types::Vector2 dpi_scale = m_windows.dpiScale(window);
        enqueue([this, window, viewport, native, pixel_size, dpi_scale] {
            m_scene->viewportCreate(viewport, {});
            m_scene->windowAttach(window, viewport, native, pixel_size, dpi_scale);
        });

        return window;
    }

    void RenderingServer::windowFree(WindowRid window)
    {
        if (!m_initialized || !window.isValid())
            return;
        assertMainThread();

        if (!m_window_rids.release(window)) {
            spdlog::warn("windowFree: window RID {:#x} is not alive", window.raw());
            return;
        }

        m_windows.destroy(window, m_submitted_frames + 1);

        ViewportRid viewport;
        if (auto it = m_window_viewports.find(window.raw()); it != m_window_viewports.end()) {
            viewport = it->second;
            m_window_viewports.erase(it);
            m_viewport_rids.release(viewport);
        }

        enqueue([this, window, viewport] {
            m_scene->windowDetach(window);
            m_scene->viewportFree(viewport);
        });
    }

    bool RenderingServer::windowSetTitle(WindowRid window, std::string_view title)
    {
        assertMainThread();
        return m_window_rids.isAlive(window) && m_windows.setTitle(window, title);
    }

    bool RenderingServer::windowSetSize(WindowRid window, core::types::Vector2i size)
    {
        assertMainThread();
        return m_window_rids.isAlive(window) && m_windows.setSize(window, size);
    }

    bool RenderingServer::windowFocus(WindowRid window)
    {
        assertMainThread();
        return m_window_rids.isAlive(window) && m_windows.focus(window);
    }

    void RenderingServer::windowSetEventCallback(WindowRid window, std::function<void(const WindowEvent &)> callback)
    {
        assertMainThread();
        if (m_window_rids.isAlive(window))
            m_windows.setEventCallback(window, std::move(callback));
    }

    bool RenderingServer::windowIsAlive(WindowRid window) const
    {
        return m_window_rids.isAlive(window);
    }

    core::types::Vector2i RenderingServer::windowGetSize(WindowRid window) const
    {
        assertMainThread();
        return m_windows.size(window);
    }

    core::types::Vector2 RenderingServer::windowGetDpiScale(WindowRid window) const
    {
        assertMainThread();
        return m_windows.dpiScale(window);
    }

    ViewportRid RenderingServer::windowGetViewport(WindowRid window) const
    {
        assertMainThread();
        auto it = m_window_viewports.find(window.raw());
        return it == m_window_viewports.end() || !m_window_rids.isAlive(window) ? ViewportRid{} : it->second;
    }

    TextureRid RenderingServer::texture2dCreate(const Texture2dDesc &desc, std::span<const std::byte> pixels)
    {
        if (!m_initialized) {
            spdlog::error("texture2dCreate called before RenderingServer::init");
            return {};
        }
        assertMainThread();

        if (desc.size.x <= 0 || desc.size.y <= 0) {
            spdlog::error("texture2dCreate: invalid size {}x{}", desc.size.x, desc.size.y);
            return {};
        }
        const std::size_t expected = static_cast<std::size_t>(desc.size.x) * static_cast<std::size_t>(desc.size.y) * 4;
        if (!pixels.empty() && pixels.size() != expected) {
            spdlog::error("texture2dCreate: expected {} bytes of RGBA8 pixels, got {}", expected, pixels.size());
            return {};
        }

        const TextureRid texture = m_texture_rids.allocate();
        m_texture_sizes[texture.raw()] = desc.size;
        enqueue([this, texture, desc, data = std::vector<std::byte>(pixels.begin(), pixels.end())] { m_scene->textureCreate(texture, desc, data); });
        return texture;
    }

    void RenderingServer::texture2dUpdate(TextureRid texture, const Region2i &region, std::span<const std::byte> pixels)
    {
        if (!alive(m_texture_rids, texture, "texture2dUpdate"))
            return;

        const core::types::Vector2i size = m_texture_sizes[texture.raw()];
        if (region.x < 0 || region.y < 0 || region.width <= 0 || region.height <= 0 || region.x + region.width > size.x || region.y + region.height > size.y) {
            spdlog::error("texture2dUpdate: region outside the texture");
            return;
        }
        if (pixels.size() != static_cast<std::size_t>(region.width) * static_cast<std::size_t>(region.height) * 4) {
            spdlog::error("texture2dUpdate: pixel data does not match the region");
            return;
        }

        enqueue([this, texture, region, data = std::vector<std::byte>(pixels.begin(), pixels.end())] { m_scene->textureUpdate(texture, region, data); });
    }

    void RenderingServer::textureFree(TextureRid texture)
    {
        if (!m_initialized || !texture.isValid())
            return;
        assertMainThread();

        for (const auto &[viewport, viewport_texture] : m_viewport_textures) {
            if (viewport_texture == texture) {
                spdlog::warn("textureFree: texture {:#x} belongs to a viewport, free the viewport instead", texture.raw());
                return;
            }
        }
        if (!m_texture_rids.release(texture)) {
            spdlog::warn("textureFree: texture RID {:#x} is not alive", texture.raw());
            return;
        }

        m_texture_sizes.erase(texture.raw());
        enqueue([this, texture] { m_scene->textureFree(texture); });
    }

    core::types::Vector2i RenderingServer::textureGetSize(TextureRid texture) const
    {
        assertMainThread();
        auto it = m_texture_sizes.find(texture.raw());
        return it == m_texture_sizes.end() || !m_texture_rids.isAlive(texture) ? core::types::Vector2i{} : it->second;
    }

    void RenderingServer::textureRequestData(TextureRid texture, std::function<void(TextureData)> on_ready)
    {
        if (!alive(m_texture_rids, texture, "textureRequestData"))
            return;

        const std::uint64_t request = m_next_readback++;
        m_readback_callbacks[request] = std::move(on_ready);
        enqueue([this, texture, request] { m_scene->textureRequestData(texture, request); });
    }

    ViewportRid RenderingServer::viewportCreate()
    {
        if (!m_initialized) {
            spdlog::error("viewportCreate called before RenderingServer::init");
            return {};
        }
        assertMainThread();

        const ViewportRid viewport = m_viewport_rids.allocate();
        const TextureRid texture = m_texture_rids.allocate();
        m_viewport_textures[viewport.raw()] = texture;
        m_viewport_sizes[viewport.raw()] = {};
        m_texture_sizes[texture.raw()] = {};
        enqueue([this, viewport, texture] { m_scene->viewportCreate(viewport, texture); });
        return viewport;
    }

    void RenderingServer::viewportSetSize(ViewportRid viewport, core::types::Vector2i size)
    {
        if (!alive(m_viewport_rids, viewport, "viewportSetSize"))
            return;

        m_viewport_sizes[viewport.raw()] = size;
        if (auto it = m_viewport_textures.find(viewport.raw()); it != m_viewport_textures.end())
            m_texture_sizes[it->second.raw()] = size;
        enqueue([this, viewport, size] { m_scene->viewportSetSize(viewport, size); });
    }

    void RenderingServer::viewportSetCanvas(ViewportRid viewport, CanvasRid canvas)
    {
        if (alive(m_viewport_rids, viewport, "viewportSetCanvas"))
            enqueue([this, viewport, canvas] { m_scene->viewportSetCanvas(viewport, canvas); });
    }

    void RenderingServer::viewportSetCanvasTransform(ViewportRid viewport, const Affine2d &transform)
    {
        if (alive(m_viewport_rids, viewport, "viewportSetCanvasTransform"))
            enqueue([this, viewport, transform] { m_scene->viewportSetCanvasTransform(viewport, transform); });
    }

    void RenderingServer::viewportSetClearColor(ViewportRid viewport, const core::types::Color &color)
    {
        if (alive(m_viewport_rids, viewport, "viewportSetClearColor"))
            enqueue([this, viewport, color = ToRgba(color)] { m_scene->viewportSetClearColor(viewport, color); });
    }

    void RenderingServer::viewportSetActive(ViewportRid viewport, bool active)
    {
        if (alive(m_viewport_rids, viewport, "viewportSetActive"))
            enqueue([this, viewport, active] { m_scene->viewportSetActive(viewport, active); });
    }

    void RenderingServer::viewportSetContentRect(ViewportRid viewport, const std::optional<Rect2> &rect)
    {
        if (alive(m_viewport_rids, viewport, "viewportSetContentRect"))
            enqueue([this, viewport, rect] { m_scene->viewportSetContentRect(viewport, rect); });
    }

    void RenderingServer::viewportSetBorderColor(ViewportRid viewport, const core::types::Color &color)
    {
        if (alive(m_viewport_rids, viewport, "viewportSetBorderColor"))
            enqueue([this, viewport, color = ToRgba(color)] { m_scene->viewportSetBorderColor(viewport, color); });
    }

    TextureRid RenderingServer::viewportGetTexture(ViewportRid viewport) const
    {
        assertMainThread();
        auto it = m_viewport_textures.find(viewport.raw());
        return it == m_viewport_textures.end() || !m_viewport_rids.isAlive(viewport) ? TextureRid{} : it->second;
    }

    core::types::Vector2i RenderingServer::viewportGetSize(ViewportRid viewport) const
    {
        assertMainThread();
        auto it = m_viewport_sizes.find(viewport.raw());
        return it == m_viewport_sizes.end() || !m_viewport_rids.isAlive(viewport) ? core::types::Vector2i{} : it->second;
    }

    void RenderingServer::viewportFree(ViewportRid viewport)
    {
        if (!m_initialized || !viewport.isValid())
            return;
        assertMainThread();

        auto texture_it = m_viewport_textures.find(viewport.raw());
        if (texture_it == m_viewport_textures.end()) {
            spdlog::warn("viewportFree: viewport {:#x} is not an offscreen viewport", viewport.raw());
            return;
        }
        if (!m_viewport_rids.release(viewport)) {
            spdlog::warn("viewportFree: viewport RID {:#x} is not alive", viewport.raw());
            return;
        }

        m_texture_rids.release(texture_it->second);
        m_texture_sizes.erase(texture_it->second.raw());
        m_viewport_textures.erase(texture_it);
        m_viewport_sizes.erase(viewport.raw());
        enqueue([this, viewport] { m_scene->viewportFree(viewport); });
    }

    CanvasRid RenderingServer::canvasCreate()
    {
        if (!m_initialized) {
            spdlog::error("canvasCreate called before RenderingServer::init");
            return {};
        }
        assertMainThread();

        const CanvasRid canvas = m_canvas_rids.allocate();
        enqueue([this, canvas] { m_scene->canvasCreate(canvas); });
        return canvas;
    }

    void RenderingServer::canvasFree(CanvasRid canvas)
    {
        if (!m_initialized || !canvas.isValid())
            return;
        assertMainThread();
        if (!m_canvas_rids.release(canvas)) {
            spdlog::warn("canvasFree: canvas RID {:#x} is not alive", canvas.raw());
            return;
        }
        enqueue([this, canvas] { m_scene->canvasFree(canvas); });
    }

    CanvasItemRid RenderingServer::canvasItemCreate(CanvasRid canvas)
    {
        if (!m_initialized) {
            spdlog::error("canvasItemCreate called before RenderingServer::init");
            return {};
        }
        assertMainThread();
        if (canvas.isValid() && !m_canvas_rids.isAlive(canvas))
            spdlog::warn("canvasItemCreate: canvas RID {:#x} is not alive", canvas.raw());

        const CanvasItemRid item = m_canvas_item_rids.allocate();
        enqueue([this, item, canvas] { m_scene->canvasItemCreate(item, canvas); });
        return item;
    }

    void RenderingServer::canvasItemSetParent(CanvasItemRid item, CanvasItemRid parent)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemSetParent"))
            enqueue([this, item, parent] { m_scene->canvasItemSetParent(item, parent); });
    }

    void RenderingServer::canvasItemSetTransform(CanvasItemRid item, const Affine2d &transform)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemSetTransform"))
            enqueue([this, item, transform] { m_scene->canvasItemSetTransform(item, transform); });
    }

    void RenderingServer::canvasItemSetZIndex(CanvasItemRid item, int z_index)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemSetZIndex"))
            enqueue([this, item, z_index] { m_scene->canvasItemSetZIndex(item, z_index); });
    }

    void RenderingServer::canvasItemSetVisible(CanvasItemRid item, bool visible)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemSetVisible"))
            enqueue([this, item, visible] { m_scene->canvasItemSetVisible(item, visible); });
    }

    void RenderingServer::canvasItemSetModulate(CanvasItemRid item, const core::types::Color &color)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemSetModulate"))
            enqueue([this, item, color = ToRgba(color)] { m_scene->canvasItemSetModulate(item, color); });
    }

    void RenderingServer::canvasItemClear(CanvasItemRid item)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemClear"))
            enqueue([this, item] { m_scene->canvasItemClear(item); });
    }

    void RenderingServer::canvasItemAddRect(CanvasItemRid item, const Rect2 &rect, const core::types::Color &color)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemAddRect"))
            enqueue([this, item, command = CanvasRectCommand{ .rect = rect, .color = ToRgba(color) }] { m_scene->canvasItemAddCommand(item, command); });
    }

    void RenderingServer::canvasItemAddTextureRect(CanvasItemRid item, const Rect2 &rect, TextureRid texture, const TextureRectOptions &options)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemAddTextureRect"))
            enqueue([this, item, command = CanvasTextureRectCommand{ .rect = rect, .texture = texture, .options = options }] {
                m_scene->canvasItemAddCommand(item, command);
            });
    }

    void RenderingServer::canvasItemAddPolygon(CanvasItemRid item, std::span<const core::types::Vector2> points, std::span<const core::types::Color> colors)
    {
        if (!alive(m_canvas_item_rids, item, "canvasItemAddPolygon"))
            return;

        CanvasPolygonCommand command{ .points = { points.begin(), points.end() } };
        command.colors.reserve(colors.size());
        for (const core::types::Color &color : colors) command.colors.push_back(ToRgba(color));
        enqueue([this, item, command = std::move(command)]() mutable { m_scene->canvasItemAddCommand(item, std::move(command)); });
    }

    void RenderingServer::canvasItemAddPolyline(
        CanvasItemRid item, std::span<const core::types::Vector2> points, const core::types::Color &color, float width, bool closed)
    {
        if (!alive(m_canvas_item_rids, item, "canvasItemAddPolyline"))
            return;

        CanvasPolylineCommand command{ .points = { points.begin(), points.end() }, .color = ToRgba(color), .width = width, .closed = closed };
        enqueue([this, item, command = std::move(command)]() mutable { m_scene->canvasItemAddCommand(item, std::move(command)); });
    }

    void RenderingServer::canvasItemPushClip(CanvasItemRid item, const Rect2 &clip)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemPushClip"))
            enqueue([this, item, clip] { m_scene->canvasItemAddCommand(item, CanvasClipPushCommand{ .rect = clip }); });
    }

    void RenderingServer::canvasItemPopClip(CanvasItemRid item)
    {
        if (alive(m_canvas_item_rids, item, "canvasItemPopClip"))
            enqueue([this, item] { m_scene->canvasItemAddCommand(item, CanvasClipPopCommand{}); });
    }

    void RenderingServer::canvasItemFree(CanvasItemRid item)
    {
        if (!m_initialized || !item.isValid())
            return;
        assertMainThread();
        if (!m_canvas_item_rids.release(item)) {
            spdlog::warn("canvasItemFree: canvas item RID {:#x} is not alive", item.raw());
            return;
        }
        enqueue([this, item] { m_scene->canvasItemFree(item); });
    }

    bool RenderingServer::legacyRendererEnabled() const noexcept
    {
        return m_initialized && m_legacy_renderer;
    }

    SDL_Window *RenderingServer::legacySdlWindow(WindowRid window) const
    {
        assertMainThread();
        return m_windows.sdlWindow(window);
    }

    RenderingPlatform &RenderingServer::platform()
    {
        if (!m_platform)
            throw std::logic_error("RenderingServer is not initialized");
        return *m_platform;
    }

    std::size_t RenderingServer::pendingCommandCount() const noexcept
    {
        return m_commands.pendingCount();
    }

    FrameStats RenderingServer::lastFrameStats() const
    {
        return m_stats;
    }

    void RenderingServer::assertMainThread() const
    {
        assert(!m_initialized || std::this_thread::get_id() == m_main_thread);
    }
} // namespace atmo::server::rendering
