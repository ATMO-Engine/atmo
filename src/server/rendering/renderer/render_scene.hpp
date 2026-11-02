#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "server/rendering/platform/rendering_platform.hpp"
#include "server/rendering/renderer/canvas.hpp"
#include "server/rendering/renderer/canvas_renderer.hpp"
#include "server/rendering/renderer/render_graph.hpp"
#include "server/rendering/rid.hpp"
#include "server/rendering/types.hpp"

namespace atmo::server::rendering
{
    class RenderScene
    {
    public:
        explicit RenderScene(RenderingPlatform &platform);
        ~RenderScene();

        RenderScene(const RenderScene &) = delete;
        RenderScene &operator=(const RenderScene &) = delete;

        void textureCreate(TextureRid rid, const Texture2dDesc &desc, std::span<const std::byte> pixels);
        void textureUpdate(TextureRid rid, const Region2i &region, std::span<const std::byte> pixels);
        void textureFree(TextureRid rid);
        void textureRequestData(TextureRid rid, std::uint64_t request);

        void viewportCreate(ViewportRid rid, TextureRid texture);
        void viewportSetSize(ViewportRid rid, core::types::Vector2i size);
        void viewportSetCanvas(ViewportRid rid, CanvasRid canvas);
        void viewportAttachCanvas(ViewportRid rid, CanvasRid canvas, int layer);
        void viewportDetachCanvas(ViewportRid rid, CanvasRid canvas);
        void viewportSetCanvasTransform(ViewportRid rid, const Affine2d &transform);
        void viewportSetClearColor(ViewportRid rid, const ColorRgba &color);
        void viewportSetActive(ViewportRid rid, bool active);
        void viewportSetContentRect(ViewportRid rid, const std::optional<Rect2> &rect);
        void viewportSetBorderColor(ViewportRid rid, const ColorRgba &color);
        void viewportSetPostProcess(ViewportRid rid, std::vector<MaterialRid> materials);
        void viewportFree(ViewportRid rid);

        void windowAttach(WindowRid rid, ViewportRid viewport, const NativeWindow &window, core::types::Vector2i pixel_size, core::types::Vector2 dpi_scale);
        void windowResize(WindowRid rid, core::types::Vector2i pixel_size, core::types::Vector2 dpi_scale);
        void windowDetach(WindowRid rid);

        void canvasCreate(CanvasRid rid);
        void canvasFree(CanvasRid rid);

        void canvasItemCreate(CanvasItemRid rid, CanvasRid canvas);
        void canvasItemSetParent(CanvasItemRid rid, CanvasItemRid parent);
        void canvasItemSetTransform(CanvasItemRid rid, const Affine2d &transform);
        void canvasItemSetZIndex(CanvasItemRid rid, int z_index);
        void canvasItemSetVisible(CanvasItemRid rid, bool visible);
        void canvasItemSetModulate(CanvasItemRid rid, const ColorRgba &color);
        void canvasItemSetMaterial(CanvasItemRid rid, MaterialRid material);
        void canvasItemSetCustom(CanvasItemRid rid, const ColorRgba &custom);
        void canvasItemClear(CanvasItemRid rid);
        void canvasItemAddCommand(CanvasItemRid rid, CanvasCommand command);
        void canvasItemAddCommands(CanvasItemRid rid, std::vector<CanvasCommand> commands);
        void canvasItemSetCommands(CanvasItemRid rid, std::vector<CanvasCommand> commands);
        void canvasItemFree(CanvasItemRid rid);

        void shaderCreate(ShaderRid rid, const ShaderDesc &desc);
        void shaderFree(ShaderRid rid);
        void materialSetData(MaterialRid rid, ShaderRid shader, MaterialData data);
        void materialFree(MaterialRid rid);
        void setTime(float seconds) noexcept;

        void render(FrameStats &stats);
        std::vector<std::pair<std::uint64_t, TextureData>> processReadbacks();

        [[nodiscard]] std::size_t windowCount() const noexcept;
        [[nodiscard]] std::size_t canvasItemCount() const noexcept;
        [[nodiscard]] const CanvasRenderer &canvasRenderer() const noexcept;

    private:
        struct CanvasLayer {
            CanvasRid canvas;
            int layer = 0;
        };

        struct Viewport {
            core::types::Vector2i size{};
            CanvasRid canvas;
            std::vector<CanvasLayer> layers;
            Affine2d transform;
            ColorRgba clear{ 0.0f, 0.0f, 0.0f, 1.0f };
            ColorRgba border{ 0.0f, 0.0f, 0.0f, 1.0f };
            std::optional<Rect2> content;
            TextureRid texture;
            bool active = true;
            std::vector<MaterialRid> post;
            std::vector<TextureEntry> post_targets;
        };

        struct Window {
            SwapchainHandle swapchain;
            ViewportRid viewport;
            core::types::Vector2 dpi_scale{ 1.0f, 1.0f };
        };

        struct Pass {
            std::string name;
            ColorAttachment attachment;
            std::optional<std::size_t> job;
            std::optional<CanvasRenderer::PostDraw> post;
            std::optional<std::size_t> overlay;
            std::vector<TextureHandle> sampled;
            TextureFormat format = TextureFormat::Undefined;
        };

        CanvasItemData *item(CanvasItemRid rid);
        Viewport *viewport(ViewportRid rid);
        void destroyTexture(TextureEntry &entry) noexcept;
        void resizeViewportTarget(Viewport &viewport);
        void addPass(
            std::string name, Viewport &target, TextureHandle attachment, core::types::Vector2i size, const Affine2d &to_pixels, TextureFormat format,
            FrameStats &stats);
        void ensurePostTargets(Viewport &target, std::size_t count, core::types::Vector2i size);
        void destroyShaderModule(ShaderEntry &entry) noexcept;

        RenderingPlatform &m_platform;
        CanvasStorage m_storage;
        CanvasRenderer m_canvas;
        RenderGraph m_graph;
        std::unordered_map<std::uint32_t, Viewport> m_viewports;
        std::unordered_map<std::uint32_t, Window> m_windows;
        std::vector<std::pair<std::uint64_t, TextureRid>> m_readbacks;
        std::vector<Pass> m_passes;
        std::vector<SwapchainHandle> m_acquired;
    };
} // namespace atmo::server::rendering
