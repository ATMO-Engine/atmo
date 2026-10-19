#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "SDL3/SDL_video.h"
#include "core/types.hpp"
#include "server/rendering/command_queue.hpp"
#include "server/rendering/platform/rendering_platform.hpp"
#include "server/rendering/render_thread.hpp"
#include "server/rendering/rid.hpp"
#include "server/rendering/rid_allocator.hpp"
#include "server/rendering/types.hpp"
#include "server/rendering/window/window_system.hpp"

namespace atmo::server::rendering
{
    class RenderScene;

    class RenderingServer
    {
    public:
        static RenderingServer &Instance();

        ~RenderingServer();

        RenderingServer(const RenderingServer &) = delete;
        RenderingServer &operator=(const RenderingServer &) = delete;

        bool init(const RenderingSettings &settings = {});
        void shutdown();
        [[nodiscard]] bool isInitialized() const noexcept;
        [[nodiscard]] PlatformKind platformKind() const;
        [[nodiscard]] bool renderThreadActive() const noexcept;

        void pollEvents();
        void sync();
        void waitIdle();

        WindowRid windowCreate(const WindowDesc &desc);
        void windowFree(WindowRid window);
        bool windowSetTitle(WindowRid window, std::string_view title);
        bool windowSetSize(WindowRid window, core::types::Vector2i size);
        bool windowFocus(WindowRid window);
        void windowSetEventCallback(WindowRid window, std::function<void(const WindowEvent &)> callback);
        [[nodiscard]] bool windowIsAlive(WindowRid window) const;
        [[nodiscard]] core::types::Vector2i windowGetSize(WindowRid window) const;
        [[nodiscard]] core::types::Vector2 windowGetDpiScale(WindowRid window) const;
        [[nodiscard]] ViewportRid windowGetViewport(WindowRid window) const;

        TextureRid texture2dCreate(const Texture2dDesc &desc, std::span<const std::byte> pixels);
        void texture2dUpdate(TextureRid texture, const Region2i &region, std::span<const std::byte> pixels);
        void textureFree(TextureRid texture);
        [[nodiscard]] core::types::Vector2i textureGetSize(TextureRid texture) const;
        void textureRequestData(TextureRid texture, std::function<void(TextureData)> on_ready);

        ViewportRid viewportCreate();
        void viewportSetSize(ViewportRid viewport, core::types::Vector2i size);
        void viewportSetCanvas(ViewportRid viewport, CanvasRid canvas);
        void viewportSetCanvasTransform(ViewportRid viewport, const Affine2d &transform);
        void viewportSetClearColor(ViewportRid viewport, const core::types::Color &color);
        void viewportSetActive(ViewportRid viewport, bool active);
        void viewportSetContentRect(ViewportRid viewport, const std::optional<Rect2> &rect);
        void viewportSetBorderColor(ViewportRid viewport, const core::types::Color &color);
        [[nodiscard]] TextureRid viewportGetTexture(ViewportRid viewport) const;
        [[nodiscard]] core::types::Vector2i viewportGetSize(ViewportRid viewport) const;
        void viewportFree(ViewportRid viewport);

        CanvasRid canvasCreate();
        void canvasFree(CanvasRid canvas);

        CanvasItemRid canvasItemCreate(CanvasRid canvas);
        void canvasItemSetParent(CanvasItemRid item, CanvasItemRid parent);
        void canvasItemSetTransform(CanvasItemRid item, const Affine2d &transform);
        void canvasItemSetZIndex(CanvasItemRid item, int z_index);
        void canvasItemSetVisible(CanvasItemRid item, bool visible);
        void canvasItemSetModulate(CanvasItemRid item, const core::types::Color &color);
        void canvasItemClear(CanvasItemRid item);
        void canvasItemAddRect(CanvasItemRid item, const Rect2 &rect, const core::types::Color &color);
        void canvasItemAddTextureRect(CanvasItemRid item, const Rect2 &rect, TextureRid texture, const TextureRectOptions &options = {});
        void canvasItemAddPolygon(CanvasItemRid item, std::span<const core::types::Vector2> points, std::span<const core::types::Color> colors);
        void canvasItemAddPolyline(
            CanvasItemRid item, std::span<const core::types::Vector2> points, const core::types::Color &color, float width = 1.0f, bool closed = false);
        void canvasItemPushClip(CanvasItemRid item, const Rect2 &clip);
        void canvasItemPopClip(CanvasItemRid item);
        void canvasItemFree(CanvasItemRid item);

        [[nodiscard]] bool legacyRendererEnabled() const noexcept;
        [[nodiscard]] SDL_Window *legacySdlWindow(WindowRid window) const;

        [[nodiscard]] RenderingPlatform &platform();
        [[nodiscard]] std::size_t pendingCommandCount() const noexcept;
        [[nodiscard]] FrameStats lastFrameStats() const;

    private:
        RenderingServer();

        void assertMainThread() const;
        void renderFrame(CommandQueue::Batch *batch);
        void completeFrames();

        template <RidKind K> bool alive(const RidAllocator<K> &allocator, Rid<K> rid, std::string_view call) const;

        template <typename F> void enqueue(F &&command, std::source_location location = std::source_location::current())
        {
            m_commands.push(std::forward<F>(command), location);
        }

        std::unique_ptr<RenderingPlatform> m_platform;
        std::unique_ptr<RenderScene> m_scene;
        WindowSystem m_windows;
        CommandQueue m_commands;
        RenderThread m_render_thread;

        RidAllocator<RidKind::Window> m_window_rids;
        RidAllocator<RidKind::Viewport> m_viewport_rids;
        RidAllocator<RidKind::Texture> m_texture_rids;
        RidAllocator<RidKind::Canvas> m_canvas_rids;
        RidAllocator<RidKind::CanvasItem> m_canvas_item_rids;

        std::unordered_map<std::uint32_t, ViewportRid> m_window_viewports;
        std::unordered_map<std::uint32_t, core::types::Vector2i> m_texture_sizes;
        std::unordered_map<std::uint32_t, TextureRid> m_viewport_textures;
        std::unordered_map<std::uint32_t, core::types::Vector2i> m_viewport_sizes;
        std::unordered_map<std::uint64_t, std::function<void(TextureData)>> m_readback_callbacks;
        std::uint64_t m_next_readback = 1;

        mutable std::mutex m_completion_mutex;
        std::vector<std::pair<std::uint64_t, TextureData>> m_completed_readbacks;
        FrameStats m_render_stats;
        FrameStats m_stats;

        std::uint64_t m_submitted_frames = 0;
        std::uint64_t m_completed_frames = 0;
        std::thread::id m_main_thread;
        bool m_legacy_renderer = true;
        bool m_initialized = false;
    };
} // namespace atmo::server::rendering
