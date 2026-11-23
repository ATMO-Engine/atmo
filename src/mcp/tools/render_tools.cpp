#include <algorithm>
#include <array>
#include <filesystem>
#include <optional>
#include <string>

#include "core/engine.hpp"
#include "core/resource/material.hpp"
#include "core/resource/resource_manager.hpp"
#include "editor/editor_manager.hpp"
#include "editor/editors/scene_editor/scene_editor.hpp"
#include "mcp/mcp_server.hpp"
#include "mcp/tools/helpers.hpp"
#include "mcp/tools/tools.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::mcp::tools
{
    namespace render_args
    {
        namespace rendering = server::rendering;

        constexpr int max_size = 4096;
        constexpr std::uint64_t capture_delay_frames = 2;

        struct SceneScreenshotArgs {
            std::optional<std::string> scene;
            std::optional<int> width;
            std::optional<int> height;
            std::optional<std::array<float, 2>> center;
            std::optional<float> zoom;
        };

        struct MaterialPreviewArgs {
            std::string path;
            std::optional<int> width;
            std::optional<int> height;
        };

        std::optional<std::string> RequireGpu()
        {
            if (rendering::RenderingServer::Instance().platformKind() == rendering::PlatformKind::Null)
                return "Nothing is rendered on the Null platform; start atmo with --rendering-platform=metal or vulkan (also in headless mode)";
            return std::nullopt;
        }

        void SendImage(const Reply &reply, const rendering::TextureData &data, glz::generic info)
        {
            auto png = EncodePngBase64(data);
            if (!png)
                return reply.error("Could not encode the image");
            info["width"] = static_cast<double>(data.size.x);
            info["height"] = static_cast<double>(data.size.y);
            ToolResult result = ToolResult::Image(std::move(*png), "image/png");
            result.addText(info.dump().value_or("{}"));
            reply.send(std::move(result));
        }
    } // namespace render_args
} // namespace atmo::mcp::tools

using namespace atmo::mcp::tools::render_args;

template <> struct glz::json_schema<SceneScreenshotArgs> {
    schema scene{ .description = "Scene file path (default: the focused scene editor tab)" };
    schema width{ .description = "Viewport width in logical units (default: the current viewport size)" };
    schema height{ .description = "Viewport height in logical units" };
    schema center{ .description = "World position shown at the center of the image" };
    schema zoom{ .description = "Editor camera zoom (1 = one world unit per logical pixel)" };
};
template <> struct glz::json_schema<MaterialPreviewArgs> {
    schema path{ .description = ".material file" };
    schema width{ .description = "Image width in pixels (default 256)" };
    schema height{ .description = "Image height in pixels (default 256)" };
};

namespace atmo::mcp::tools
{
    void RegisterRenderTools(McpServer &server)
    {
        if (server.mode() != McpServer::Mode::Editor)
            return;

        auto &tools = server.tools();

        tools.add<SceneScreenshotArgs>(
            "scene_screenshot",
            "Render a scene editor viewport (editor camera, without the editor overlays' UI) to a PNG image.",
            [&server](const SceneScreenshotArgs &args, const Reply &reply) {
                if (auto error = RequireGpu())
                    return reply.error(*error);
                auto target = ResolveScene(server, args.scene);
                if (!target)
                    return reply.error(target.error());
                auto *context = target->editor->getSceneContext();
                if (!context || !context->isReady())
                    return reply.error("The scene viewport is not ready");

                const int width = std::clamp(args.width.value_or(context->getWidth()), 1, max_size);
                const int height = std::clamp(args.height.value_or(context->getHeight()), 1, max_size);
                const core::types::Vector2 previous_position = context->cameraPosition();
                const float previous_zoom = context->cameraZoom();
                const bool move_camera = args.center || args.zoom;
                if (move_camera)
                    context->setCamera(
                        args.center ? core::types::Vector2{ (*args.center)[0], (*args.center)[1] } : previous_position, args.zoom.value_or(previous_zoom));
                context->setSizeOverride(core::types::Vector2i{ width, height });

                std::weak_ptr<editor::SceneEditor> weak_editor = target->editor;
                server.after(capture_delay_frames, [reply, weak_editor, move_camera, previous_position, previous_zoom] {
                    auto editor = weak_editor.lock();
                    if (!editor)
                        return reply.error("The scene editor was closed");
                    auto *context = editor->getSceneContext();
                    const core::types::Vector2 position = context->cameraPosition();
                    const float zoom = context->cameraZoom();
                    rendering::RenderingServer::Instance().textureRequestData(
                        context->getViewportTexture(),
                        [reply, weak_editor, move_camera, previous_position, previous_zoom, position, zoom](rendering::TextureData data) {
                            if (auto editor = weak_editor.lock()) {
                                editor->getSceneContext()->setSizeOverride(std::nullopt);
                                if (move_camera)
                                    editor->getSceneContext()->setCamera(previous_position, previous_zoom);
                            }
                            SendImage(
                                reply,
                                data,
                                glz::generic::object_t{
                                    { "center", glz::generic::array_t{ static_cast<double>(position.x), static_cast<double>(position.y) } },
                                    { "zoom", static_cast<double>(zoom) },
                                });
                        });
                });
            });

        tools.add<MaterialPreviewArgs>(
            "material_preview",
            "Render a material to a PNG image: canvas materials on a white quad over a transparency grid, viewport materials over a color test "
            "pattern.",
            [&server](const MaterialPreviewArgs &args, const Reply &reply) {
                if (auto error = RequireGpu())
                    return reply.error(*error);
                const std::filesystem::path path = ResolvePath(args.path);
                if (!std::filesystem::exists(path))
                    return reply.error("Material not found: " + args.path);

                std::string key = path.string();
                if (auto *manager = server.engine().editor())
                    if (auto editor = manager->findEditorByPath(path); editor && editor->hasFilePath())
                        key = *editor->filePath();
                auto material = core::resource::ResourceManager::GetInstance().getResource<core::resource::Material>(key);
                if (!material || !material->shader())
                    return reply.error("The material or its shader could not be loaded (see material_get and shader_status)");

                const int width = std::clamp(args.width.value_or(256), 1, max_size);
                const int height = std::clamp(args.height.value_or(256), 1, max_size);
                const float w = static_cast<float>(width);
                const float h = static_cast<float>(height);

                auto &rs = rendering::RenderingServer::Instance();
                const rendering::ViewportRid viewport = rs.viewportCreate();
                const rendering::CanvasRid canvas = rs.canvasCreate();
                const rendering::CanvasItemRid background = rs.canvasItemCreate(canvas);
                const rendering::CanvasItemRid item = rs.canvasItemCreate(canvas);
                rs.viewportSetSize(viewport, { width, height });
                rs.viewportSetCanvas(viewport, canvas);
                rs.viewportSetClearColor(viewport, core::types::Color(0.8f, 0.8f, 0.8f, 1.0f));

                const bool post = material->kind() == rendering::ShaderKind::Viewport;
                const float cell = 16.0f;
                for (float y = 0; y < h; y += cell)
                    for (float x = 0; x < w; x += cell)
                        if ((static_cast<int>(x / cell) + static_cast<int>(y / cell)) % 2 == 0)
                            rs.canvasItemAddRect(background, { { x, y }, { cell, cell } }, core::types::Color(0.6f, 0.6f, 0.6f, 1.0f));

                if (post) {
                    const std::array colors{ core::types::Color(0.9f, 0.2f, 0.2f, 1.0f),
                                             core::types::Color(0.2f, 0.8f, 0.3f, 1.0f),
                                             core::types::Color(0.2f, 0.4f, 0.9f, 1.0f),
                                             core::types::Color(1.0f, 1.0f, 1.0f, 1.0f) };
                    for (std::size_t i = 0; i < colors.size(); ++i)
                        rs.canvasItemAddRect(item, { { w * 0.1f + w * 0.2f * static_cast<float>(i), h * 0.25f }, { w * 0.15f, h * 0.5f } }, colors[i]);
                    const std::array chain{ material->rid() };
                    rs.viewportSetPostProcess(viewport, chain);
                } else {
                    rs.canvasItemAddRect(item, { { w * 0.1f, h * 0.1f }, { w * 0.8f, h * 0.8f } }, core::types::Color(1.0f, 1.0f, 1.0f, 1.0f));
                    rs.canvasItemSetMaterial(item, material->rid());
                }

                server.after(capture_delay_frames, [reply, viewport, canvas, background, item, material] {
                    auto &rs = rendering::RenderingServer::Instance();
                    rs.textureRequestData(rs.viewportGetTexture(viewport), [reply, viewport, canvas, background, item, material](rendering::TextureData data) {
                        auto &rs = rendering::RenderingServer::Instance();
                        rs.canvasItemFree(item);
                        rs.canvasItemFree(background);
                        rs.viewportFree(viewport);
                        rs.canvasFree(canvas);
                        SendImage(
                            reply, data, glz::generic::object_t{ { "kind", material->kind() == rendering::ShaderKind::Viewport ? "viewport" : "canvas" } });
                    });
                });
            });
    }
} // namespace atmo::mcp::tools
