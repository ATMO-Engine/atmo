#include "sprite_2d.hpp"
#include <unordered_map>
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/2d/camera_2d/camera_2d.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void Sprite2d::RegisterSystems(flecs::world *world)
    {
        world->system<components::Sprite2d>("Sprite2d_Sync").kind(flecs::PreUpdate).each([](flecs::entity /*e*/, components::Sprite2d &sprite) {
            if (sprite.texture_path.empty() || sprite.m_res)
                return;

            sprite.m_res = resource::ResourceManager::GetInstance().getResource<SDL_Surface>(sprite.texture_path);
            if (auto surface = sprite.m_res->get())
                sprite.texture_size = { static_cast<float>(surface->w), static_cast<float>(surface->h) };
        });

        world->system<components::Sprite2d, components::Transform2d>("Sprite2D_Render")
            .kind(flecs::OnValidate)
            .each([world](flecs::entity e, components::Sprite2d &sprite, components::Transform2d &transform) {
                SDL_Renderer *renderer = nullptr;
                SDL_Texture *texture = nullptr;

                const components::WorldContext *ctx = world->try_get<components::WorldContext>();
                if (ctx && ctx->is_editor_isolated) {
                    renderer = ctx->renderer;
                    if (!renderer || sprite.texture_path.empty() || !sprite.m_res)
                        return;

                    static std::unordered_map<SDL_Renderer *, std::unordered_map<std::string, SDL_Texture *>> s_iso_cache;
                    auto &cache = s_iso_cache[renderer];
                    auto it = cache.find(sprite.texture_path);
                    if (it == cache.end()) {
                        auto surface = sprite.m_res->get();
                        if (!surface)
                            return;
                        cache[sprite.texture_path] = SDL_CreateTextureFromSurface(renderer, surface.get());
                        it = cache.find(sprite.texture_path);
                    }
                    texture = it->second;
                } else {
                    flecs::entity root = world->lookup("_Root");
                    if (!root.is_valid() || !root.has<components::Window>())
                        return;

                    auto window = root.get_ref<components::Window>();
                    if (!window || !window->renderer_data.renderer)
                        return;

                    renderer = window->renderer_data.renderer;
                    Window window_entity(root);
                    texture = window_entity.getTextureFromHandle(sprite.texture_path);
                }

                if (!texture || !renderer)
                    return;

                const auto *cam = world->try_get<components::WorldCameraState>();
                int vp_w = 0, vp_h = 0;
                SDL_GetCurrentRenderOutputSize(renderer, &vp_w, &vp_h);

                float sx, sy, zoom;
                if (cam && cam->has_camera) {
                    zoom = cam->zoom;
                    sx = (transform.g_position.x - cam->position.x) * zoom + static_cast<float>(vp_w) * 0.5f;
                    sy = (transform.g_position.y - cam->position.y) * zoom + static_cast<float>(vp_h) * 0.5f;
                } else {
                    zoom = 1.0f;
                    sx = transform.g_position.x;
                    sy = transform.g_position.y;
                }

                const float w = sprite.texture_size.x * transform.g_scale.x * zoom;
                const float h = sprite.texture_size.y * transform.g_scale.y * zoom;

                SDL_FRect dst{ sx - w * 0.5f, sy - h * 0.5f, w, h };
                SDL_FPoint center{ dst.w * 0.5f, dst.h * 0.5f };

                SDL_RenderTextureRotated(renderer, texture, nullptr, &dst, transform.g_rotation, &center, SDL_FLIP_NONE);
            });

        world->observer<components::Sprite2d>("Sprite2D_remove").event(flecs::OnRemove).each([](flecs::entity e, components::Sprite2d &sprite) {
            sprite.m_res = nullptr;
        });
    }

    void Sprite2d::initialize()
    {
        Entity2d::initialize();

        setComponent<components::Sprite2d>({});
    }

    void Sprite2d::setTexturePath(const std::string &path)
    {
        auto sprite = p_handle.get_ref<components::Sprite2d>();
        sprite->texture_path = path;

        if (sprite->texture_path.empty())
            return;

        {
            std::unique_ptr<resource::ResourceRef<SDL_Surface>> res = resource::ResourceManager::GetInstance().getResource<SDL_Surface>(sprite->texture_path);

            sprite->m_res = std::move(res);
        }

        spdlog::debug("Loaded Sprite2D texture for entity {}: {}", p_handle.name().c_str(), sprite->texture_path);

        std::shared_ptr<SDL_Surface> surface = sprite->m_res->get();

        spdlog::debug("Sprite2D texture size: {}x{}", surface->w, surface->h);

        sprite->texture_size = { static_cast<float>(surface->w), static_cast<float>(surface->h) };
    }

    std::string_view Sprite2d::getTexturePath() const noexcept
    {
        auto sprite = p_handle.get_ref<components::Sprite2d>();
        return sprite->texture_path;
    }

    types::Vector2 Sprite2d::getTextureSize() const noexcept
    {
        auto sprite = p_handle.get_ref<components::Sprite2d>();
        return sprite->texture_size;
    }

    SDL_FRect Sprite2d::computeAABB() const
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        auto s = p_handle.get_ref<components::Sprite2d>();
        const float w = s->texture_size.x * t->g_scale.x;
        const float h = s->texture_size.y * t->g_scale.y;
        return { t->g_position.x - w * 0.5f, t->g_position.y - h * 0.5f, w, h };
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Sprite2d);
ATMO_REGISTER_COMPONENT(atmo::core::components::Sprite2d)
