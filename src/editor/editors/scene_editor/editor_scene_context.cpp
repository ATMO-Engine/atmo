#include "editor_scene_context.hpp"

#include <fstream>
#include <sstream>

#include "SDL3/SDL_render.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "glaze/glaze.hpp"
#include "glaze/json/prettify.hpp"
#include "spdlog/spdlog.h"

namespace atmo::editor
{
    EditorSceneContext::~EditorSceneContext()
    {
        destroyRenderTexture();
    }

    void EditorSceneContext::init(SDL_Renderer *renderer, int width, int height)
    {
        m_renderer = renderer;
        m_width = width;
        m_height = height;

        m_world.reset();
        m_world.init_builtin_components();

        core::components::register_core_components(m_world);

        createRenderTexture(width, height);
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

        m_ready = true;
        spdlog::debug("EditorSceneContext: initialized isolated world ({}x{})", width, height);
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
        SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
        SDL_RenderClear(m_renderer);

        m_world.progress(delta_time);

        SDL_FlushRenderer(m_renderer);
        SDL_SetRenderTarget(m_renderer, nullptr);

        SDL_SetRenderDrawColorFloat(m_renderer, r, g, b, a);
        SDL_SetRenderDrawBlendMode(m_renderer, blend);
    }

    void EditorSceneContext::resize(int width, int height)
    {
        if (m_width == width && m_height == height)
            return;

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

        for (auto &child : m_scene->getChildren()) child.destroy();

        core::ecs::entities::EntityData data;
        if (auto err = glz::read_json(data, json); err) {
            spdlog::error("EditorSceneContext: JSON parse error while loading scene");
            return;
        }

        // Preserve the Box2D world ID created in initialize() — it must not be
        // overwritten by JSON-serialized pointer values from a different session.
        const auto saved_world_id = m_scene->getWorldId();

        m_scene->deserializeInWorld(data, &m_world);

        auto scene_comp = m_scene->getHandle().get_ref<core::components::Scene>();
        if (scene_comp)
            scene_comp->world_id = saved_world_id;
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
} // namespace atmo::editor
