#include "sprite_2d.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void Sprite2d::RegisterComponents(flecs::world *world)
    {
        world->component<components::Sprite2d>();
    }

    void Sprite2d::RegisterSystems(flecs::world *world)
    {
        world->system<components::Sprite2d, components::Transform2d>("Sprite2D_UpdateDestRect")
            .kind(flecs::OnValidate)
            .each([](flecs::entity e, components::Sprite2d &sprite, components::Transform2d &transform) {
                sprite.m_dest_rect.x = transform.g_position.x + transform.position.x;
                sprite.m_dest_rect.y = transform.g_position.y + transform.position.y;
                sprite.m_dest_rect.w = sprite.texture_size.x * (transform.g_scale.x * transform.scale.x);
                sprite.m_dest_rect.h = sprite.texture_size.y * (transform.g_scale.y * transform.scale.y);
            });

        world->system<components::Sprite2d, components::Transform2d, components::Window>("Sprite2D_Render")
            .kind(flecs::OnValidate)
            .term_at(2)
            .up()
            .each([](flecs::iter &it, size_t i, components::Sprite2d &sprite, components::Transform2d &transform, components::Window &window) {
                flecs::entity window_src = it.src(2);
                if (!window_src) {
                    window_src = it.entity(i);
                }

                Window window_entity(window_src);

                SDL_Texture *texture = window_entity.getTextureFromHandle(sprite.m_handle);
                if (!texture)
                    return;

                SDL_RenderTextureRotated(
                    window.renderer_data.renderer, texture, nullptr, &sprite.m_dest_rect, transform.g_rotation + transform.rotation, nullptr, SDL_FLIP_NONE);
            });

        world->observer<components::Sprite2d>("Sprite2D_remove").event(flecs::OnRemove).each([](flecs::entity e, components::Sprite2d &sprite) {
            if (sprite.texture_path.empty())
                return;

            resource::ResourceRef<SDL_Surface> res = resource::ResourceManager::GetInstance().getResource<SDL_Surface>(sprite.m_handle.assetId);

            res.unpin();

            spdlog::debug("Unpinned Sprite2D texture for entity {}: {}", e.name().c_str(), sprite.texture_path);
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

        sprite->m_handle = resource::Handle<SDL_Surface>{ .assetId = sprite->texture_path };

        resource::ResourceRef<SDL_Surface> res = resource::ResourceManager::GetInstance().getResource<SDL_Surface>(sprite->m_handle.assetId);

        res.pin();

        spdlog::debug("Loaded Sprite2D texture for entity {}: {}", p_handle.name().c_str(), sprite->texture_path);

        std::shared_ptr<SDL_Surface> surface = res.get();

        spdlog::debug("Sprite2D texture size: {}x{}", surface->w, surface->h);

        sprite->texture_size = { static_cast<float>(surface->w), static_cast<float>(surface->h) };
    }

    std::string_view Sprite2d::getTexturePath() const noexcept
    {
        auto sprite = p_handle.get_ref<components::Sprite2d>();
        return sprite->texture_path;
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Sprite2d);
