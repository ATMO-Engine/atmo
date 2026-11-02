#include "editor_scene_context.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>

#include "core/ecs/components.hpp"
#include "core/ecs/entities/2d/camera_2d/camera_2d.hpp"
#include "core/ecs/entities/2d/sprite_2d/sprite_2d.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "glaze/glaze.hpp"
#include "glaze/json/prettify.hpp"
#include "project/project_manager.hpp"
#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

namespace atmo::editor
{
    namespace
    {
        constexpr int overlay_z_index = 1 << 21;
    } // namespace

    EditorSceneContext::~EditorSceneContext()
    {
        auto &rendering_server = server::rendering::RenderingServer::Instance();
        rendering_server.canvasItemFree(m_overlay);
        rendering_server.viewportFree(m_viewport);
        rendering_server.canvasFree(m_canvas);
    }

    void EditorSceneContext::init(core::types::Vector2 dpi_scale)
    {
        m_dpi_scale = dpi_scale;
        m_width = project::ProjectManager::GetSettings().window.size.x;
        m_height = project::ProjectManager::GetSettings().window.size.y;

        m_world.reset();
        m_world.init_builtin_components();

        core::components::register_core_components(m_world);

        auto &rendering_server = server::rendering::RenderingServer::Instance();
        m_canvas = rendering_server.canvasCreate();
        m_viewport = rendering_server.viewportCreate();
        m_overlay = rendering_server.canvasItemCreate(m_canvas);
        rendering_server.canvasItemSetZIndex(m_overlay, overlay_z_index);
        rendering_server.viewportSetCanvas(m_viewport, m_canvas);
        rendering_server.viewportSetClearColor(m_viewport, project::ProjectManager::GetSettings().window.background_color);

        m_world.set<core::components::WorldContext>({
            .is_editor_isolated = true,
            .canvas = m_canvas,
            .viewport = m_viewport,
            .viewport_size = { static_cast<float>(m_width), static_cast<float>(m_height) },
            .viewport_dpi = m_dpi_scale,
        });
        applyViewportSize();

        core::ecs::EntityRegistry::RegisterSystemsForWorld(&m_world);

        auto scene = core::ecs::EntityRegistry::CreateIn(&m_world, "Entity::Scene");
        if (!scene) {
            spdlog::error("EditorSceneContext: failed to create isolated Scene entity");
            return;
        }
        m_scene = std::static_pointer_cast<core::ecs::entities::Scene>(scene);

        m_world.system("EditorViewportCamera_UpdateWorldState").kind(flecs::PreUpdate).run([this](flecs::iter &it) {
            const auto *scene_camera = it.world().try_get<core::components::WorldCameraState>();
            std::vector<server::rendering::MaterialRid> post =
                scene_camera && scene_camera->has_camera ? scene_camera->post_process : std::vector<server::rendering::MaterialRid>{};
            it.world().set<core::components::WorldCameraState>(
                { .has_camera = true, .zoom = m_cam_zoom, .position = m_cam_position, .post_process = std::move(post) });
        });

        m_ready = true;
        spdlog::debug("EditorSceneContext: initialized isolated world ({}x{})", m_width, m_height);
    }

    void EditorSceneContext::tick(float delta_time)
    {
        if (!m_ready)
            return;

        m_world.progress(delta_time);
        drawOverlays();
    }

    void EditorSceneContext::applyViewportSize()
    {
        const core::types::Vector2i pixels{
            std::max(1, static_cast<int>(std::lround(static_cast<float>(m_width) * m_dpi_scale.x))),
            std::max(1, static_cast<int>(std::lround(static_cast<float>(m_height) * m_dpi_scale.y))),
        };
        server::rendering::RenderingServer::Instance().viewportSetSize(m_viewport, pixels);

        if (auto *ctx = m_world.try_get_mut<core::components::WorldContext>()) {
            ctx->viewport_size = { static_cast<float>(m_width), static_cast<float>(m_height) };
            ctx->viewport_dpi = m_dpi_scale;
        }
    }

    void EditorSceneContext::resize(int width, int height)
    {
        if (m_width == width && m_height == height)
            return;

        spdlog::info("Resizing scene editor to: {}, {}", width, height);

        m_width = width;
        m_height = height;
        applyViewportSize();
    }

    void EditorSceneContext::setDpiScale(core::types::Vector2 dpi_scale)
    {
        if (dpi_scale.x == m_dpi_scale.x && dpi_scale.y == m_dpi_scale.y)
            return;
        m_dpi_scale = dpi_scale;
        applyViewportSize();
    }

    server::rendering::TextureRid EditorSceneContext::getViewportTexture() const
    {
        return server::rendering::RenderingServer::Instance().viewportGetTexture(m_viewport);
    }

    std::string EditorSceneContext::saveSceneToJson() const
    {
        if (!m_scene || !m_scene->isAlive())
            return "{}";

        const auto data = m_scene->serialize();
        auto result = glz::write_json(data);
        if (!result)
            return "{}";

        return glz::prettify_json(*result);
    }

    void EditorSceneContext::saveSceneToFile(const std::string &path) const
    {
        const std::string json = saveSceneToJson();
        std::ofstream file(path);
        if (!file.is_open()) {
            spdlog::error("EditorSceneContext: cannot open file for writing: {}", path);
            return;
        }
        file << json;
        spdlog::info("EditorSceneContext: saved scene to {}", path);
    }

    void EditorSceneContext::loadSceneFromJson(const std::string &json)
    {
        if (!m_scene || !m_scene->isAlive())
            return;

        if (!m_scene->loadFromJson(json, &m_world))
            spdlog::error("EditorSceneContext: failed to load scene");
    }

    void EditorSceneContext::loadSceneFromFile(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open()) {
            spdlog::error("EditorSceneContext: cannot open file for reading: {}", path);
            return;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        loadSceneFromJson(ss.str());
    }

    void EditorSceneContext::pan(core::types::Vector2 delta_screen)
    {
        float inv_zoom = 1.0f / m_cam_zoom;
        m_cam_position.x -= delta_screen.x * inv_zoom;
        m_cam_position.y -= delta_screen.y * inv_zoom;
    }

    void EditorSceneContext::zoom(float factor, core::types::Vector2 pivot_screen)
    {
        const core::types::Vector2 world_pivot = screenToWorld(pivot_screen);
        const float new_zoom = std::clamp(m_cam_zoom * factor, 0.05f, 20.0f);
        m_cam_zoom = new_zoom;

        // Keep the world point under the pivot fixed after zoom.
        const float hw = static_cast<float>(m_width) * 0.5f;
        const float hh = static_cast<float>(m_height) * 0.5f;
        m_cam_position = {
            world_pivot.x - (pivot_screen.x - hw) / new_zoom,
            world_pivot.y - (pivot_screen.y - hh) / new_zoom,
        };
    }

    core::types::Vector2 EditorSceneContext::screenToWorld(core::types::Vector2 screen) const
    {
        return {
            (screen.x - static_cast<float>(m_width) * 0.5f) / m_cam_zoom + m_cam_position.x,
            (screen.y - static_cast<float>(m_height) * 0.5f) / m_cam_zoom + m_cam_position.y,
        };
    }

    core::types::Vector2 EditorSceneContext::worldToScreen(core::types::Vector2 world) const
    {
        return {
            (world.x - m_cam_position.x) * m_cam_zoom + static_cast<float>(m_width) * 0.5f,
            (world.y - m_cam_position.y) * m_cam_zoom + static_cast<float>(m_height) * 0.5f,
        };
    }

    void EditorSceneContext::drawOverlays()
    {
        const float pixel = 1.0f / m_cam_zoom;
        const core::types::Vector2 top_left = screenToWorld({ 0.0f, 0.0f });
        const core::types::Vector2 bottom_right = screenToWorld({ static_cast<float>(m_width), static_cast<float>(m_height) });
        const auto &win_size = project::ProjectManager::GetSettings().window.size;
        const float window_w = static_cast<float>(win_size.x);
        const float window_h = static_cast<float>(win_size.y);

        const std::array<core::types::Vector2, 4> bounds{ { { 0.0f, 0.0f }, { window_w, 0.0f }, { window_w, window_h }, { 0.0f, window_h } } };
        const std::array<core::types::Vector2, 2> x_axis{ { { top_left.x, 0.0f }, { bottom_right.x, 0.0f } } };
        const std::array<core::types::Vector2, 2> y_axis{ { { 0.0f, top_left.y }, { 0.0f, bottom_right.y } } };

        server::rendering::CanvasCommandWriter writer;
        writer.addPolyline(bounds, core::types::Color(1.0f, 1.0f, 1.0f, 120.0f / 255.0f), pixel, true);
        writer.addPolyline(x_axis, core::types::Color(220.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f, 200.0f / 255.0f), pixel);
        writer.addPolyline(y_axis, core::types::Color(60.0f / 255.0f, 200.0f / 255.0f, 60.0f / 255.0f, 200.0f / 255.0f), pixel);
        server::rendering::RenderingServer::Instance().canvasItemSetCommands(m_overlay, std::move(writer));
    }
} // namespace atmo::editor
