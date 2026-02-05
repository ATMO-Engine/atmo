#include "core/ecs/ecs.hpp"
#include "core/ecs/ecs_registry.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "flecs.h"
#include "impl/window.hpp"
#include "project/project_manager.hpp"

atmo::core::ecs::Prefab createSprite2dPrefab(flecs::world world)
{
    using namespace atmo::core;
    auto sprite2DPrefab = ecs::Prefab(world, "sprite_2d").set(components::Transform2d{}).set(components::Sprite2D{});

    world.observer<components::Sprite2D>("Sprite2D_LoadTexture").event(flecs::OnSet).each([](flecs::entity e, components::Sprite2D &sprite) {
        if (sprite.texture_path.empty())
            return;

        sprite.m_handle = resource::Handle<SDL_Surface>{ .assetId = sprite.texture_path };

        resource::ResourceRef<SDL_Surface> res = resource::ResourceManager::GetInstance().getResource<SDL_Surface>(sprite.m_handle.assetId);

        res.pin();

        spdlog::debug("Loaded Sprite2D texture for entity {}: {}", e.name().c_str(), sprite.texture_path);

        std::shared_ptr<SDL_Surface> surface = res.get();

        spdlog::debug("Sprite2D texture size: {}x{}", surface->w, surface->h);

        sprite.texture_size = { static_cast<float>(surface->w), static_cast<float>(surface->h) };
    });

    world.system<components::Sprite2D, components::Transform2d>("Sprite2D_UpdateDestRect")
        .kind(flecs::OnValidate)
        .each([](flecs::entity e, components::Sprite2D &sprite, components::Transform2d &transform) {
            sprite.m_dest_rect.x = transform.g_position.x + transform.position.x;
            sprite.m_dest_rect.y = transform.g_position.y + transform.position.y;
            sprite.m_dest_rect.w = sprite.texture_size.x * (transform.g_scale.x * transform.scale.x);
            sprite.m_dest_rect.h = sprite.texture_size.y * (transform.g_scale.y * transform.scale.y);
        });

    world.system<components::Sprite2D, components::Transform2d, ComponentManager::Managed, components::Window>("Sprite2D_Render")
        .kind(flecs::OnValidate)
        .term_at(2)
        .up()
        .term_at(3)
        .up()
        .each([](flecs::entity e,
                 components::Sprite2D &sprite,
                 components::Transform2d &transform,
                 ComponentManager::Managed &manager,
                 components::Window &window) {
            auto wm = static_cast<atmo::impl::WindowManager *>(manager.ptr);

            SDL_Texture *texture = wm->getTextureFromHandle(sprite.m_handle);
            if (!texture)
                return;

            SDL_RenderTextureRotated(
                wm->getRenderer(), texture, nullptr, &sprite.m_dest_rect, transform.g_rotation + transform.rotation, nullptr, SDL_FLIP_NONE);
        });

    world.observer<components::Sprite2D>("Sprite2D_remove").event(flecs::OnRemove).each([](flecs::entity e, components::Sprite2D &sprite) {
        if (sprite.texture_path.empty())
            return;

        resource::ResourceRef<SDL_Surface> res = resource::ResourceManager::GetInstance().getResource<SDL_Surface>(sprite.m_handle.assetId);

        res.unpin();

        spdlog::debug("Unpinned Sprite2D texture for entity {}: {}", e.name().c_str(), sprite.texture_path);
    });

    return sprite2DPrefab;
}

namespace
{
    static int _ = [] {
        atmo::core::ecs::Registry::AddPrefabLoader(&createSprite2dPrefab);
        return 0;
    }();
} // namespace
