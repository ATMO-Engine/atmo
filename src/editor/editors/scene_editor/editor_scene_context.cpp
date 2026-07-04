#include "editor_scene_context.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>

#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/2d/camera_2d/camera_2d.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "glaze/glaze.hpp"
#include "glaze/json/prettify.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

namespace atmo::editor
{
    EditorSceneContext::~EditorSceneContext()
    {
        destroyRenderTexture();
    }

    void EditorSceneContext::init(SDL_Renderer *renderer)
    {
        m_renderer = renderer;
        m_width = project::ProjectManager::GetSettings().window.size.x;
        m_height = project::ProjectManager::GetSettings().window.size.y;

        m_world.reset();
        m_world.init_builtin_components();

        core::components::register_core_components(m_world);

        createRenderTexture(m_width, m_height);
        m_world.set<core::components::WorldContext>({
            .is_editor_isolated = true,
            .renderer = m_renderer,
            .render_target = m_render_texture,
        });

        core::ecs::EntityRegistry::RegisterSystemsForWorld(&m_world);

        auto scene = core::ecs::EntityRegistry::CreateIn(&m_world, "Entity::Scene");
        if (!scene) {
            spdlog::error("EditorSceneContext: failed to create isolated Scene entity");
            return;
        }
        m_scene = std::static_pointer_cast<core::ecs::entities::Scene>(scene);

        m_world.system("EditorViewportCamera_UpdateWorldState").kind(flecs::PreUpdate).run([this](flecs::iter &it) {
            it.world().set<core::components::WorldCameraState>({ .has_camera = true, .zoom = m_cam_zoom, .position = m_cam_position });
        });

        m_ready = true;
        spdlog::debug("EditorSceneContext: initialized isolated world ({}x{})", m_width, m_height);
    }

    void EditorSceneContext::tick(float delta_time, SDL_Renderer *renderer)
    {
        if (!m_ready || !renderer || !m_render_texture)
            return;

        if (renderer != m_renderer) {
            m_renderer = renderer;
            auto *ctx = m_world.try_get_mut<core::components::WorldContext>();
            if (ctx)
                ctx->renderer = m_renderer;
        }

        float r, g, b, a;
        SDL_GetRenderDrawColorFloat(m_renderer, &r, &g, &b, &a);
        SDL_BlendMode blend;
        SDL_GetRenderDrawBlendMode(m_renderer, &blend);

        SDL_SetRenderTarget(m_renderer, m_render_texture);
        SDL_Color bg_color = project::ProjectManager::GetSettings().window.background_color.toInt<SDL_Color>();
        SDL_SetRenderDrawColor(m_renderer, bg_color.r, bg_color.g, bg_color.b, 255);
        SDL_RenderClear(m_renderer);

        m_world.progress(delta_time);

        drawOverlays();

        SDL_SetRenderTarget(m_renderer, nullptr);

        SDL_SetRenderDrawColorFloat(m_renderer, r, g, b, a);
        SDL_SetRenderDrawBlendMode(m_renderer, blend);
    }

    void EditorSceneContext::resize(int width, int height)
    {
        if (m_width == width && m_height == height)
            return;

        spdlog::info("Resizing scene editor to: {}, {}", width, height);

        m_width = width;
        m_height = height;
        destroyRenderTexture();
        createRenderTexture(width, height);

        auto *ctx = m_world.try_get_mut<core::components::WorldContext>();
        if (ctx)
            ctx->render_target = m_render_texture;
    }

    void EditorSceneContext::createRenderTexture(int width, int height)
    {
        m_render_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
        if (!m_render_texture)
            spdlog::error("EditorSceneContext: SDL_CreateTexture failed: {}", SDL_GetError());
    }

    void EditorSceneContext::destroyRenderTexture()
    {
        if (m_render_texture) {
            SDL_DestroyTexture(m_render_texture);
            m_render_texture = nullptr;
        }
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
        if (!m_renderer)
            return;

        const float z = m_cam_zoom;
        const auto &cam_pos = m_cam_position;
        const float hw = static_cast<float>(m_width) * 0.5f;
        const float hh = static_cast<float>(m_height) * 0.5f;

        // Screen-space position of the world origin (0, 0).
        const float ox = -cam_pos.x * z + hw;
        const float oy = -cam_pos.y * z + hh;

        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

        // Canvas bounds — outline of the game window size from project settings.
        const auto &win_size = project::ProjectManager::GetSettings().window.size;
        SDL_FRect canvas{ ox, oy, static_cast<float>(win_size.x) * z, static_cast<float>(win_size.y) * z };
        SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 120);
        SDL_RenderRect(m_renderer, &canvas);

        // X axis — red horizontal line at world y = 0.
        SDL_SetRenderDrawColor(m_renderer, 220, 60, 60, 200);
        SDL_RenderLine(m_renderer, 0.f, oy, static_cast<float>(m_width), oy);

        // Y axis — green vertical line at world x = 0.
        SDL_SetRenderDrawColor(m_renderer, 60, 200, 60, 200);
        SDL_RenderLine(m_renderer, ox, 0.f, ox, static_cast<float>(m_height));

        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
    }
} // namespace atmo::editor
