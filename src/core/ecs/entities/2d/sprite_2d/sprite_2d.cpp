#include "sprite_2d.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "spdlog/spdlog.h"

#include "SDL3/SDL_rect.h"

namespace atmo::core::ecs::entities
{
    void Sprite2d::RegisterComponents(flecs::world *world)
    {
        world->component<components::Sprite2d>();
    }

    void Sprite2d::RegisterSystems(flecs::world *world)
    {
        world->system<components::Sprite2d, components::Transform2d>("Sprite2D_Render")
            .kind(flecs::OnValidate)
            .each([world](flecs::entity e, components::Sprite2d &sprite, components::Transform2d &transform) {
                flecs::entity root = world->lookup("_Root");
                if (!root.is_valid() || !root.has<components::Window>())
                    return;

                auto window = root.get_ref<components::Window>();
                if (!window || !window->renderer_data.renderer)
                    return;

                Window window_entity(root);

                SDL_Texture *texture = window_entity.getTextureFromHandle(sprite.texture_path);
                if (!texture)
                    return;

                const float w = sprite.texture_size.x * transform.g_scale.x;
                const float h = sprite.texture_size.y * transform.g_scale.y;

                SDL_FRect dst{ transform.g_position.x - (w * 0.5f), transform.g_position.y - (h * 0.5f), w, h };
                SDL_FPoint center{ dst.w * 0.5f, dst.h * 0.5f };

                SDL_RenderTextureRotated(window->renderer_data.renderer, texture, nullptr, &dst, transform.g_rotation, &center, SDL_FLIP_NONE);
            });

        world->observer<components::Sprite2d>("Sprite2D_remove").event(flecs::OnRemove).each([](flecs::entity e, components::Sprite2d &sprite) {
            // potential cleanup
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

        resource::ResourceRef<SDL_Surface> res = resource::ResourceManager::GetInstance().getResource<SDL_Surface>(sprite->texture_path);

        sprite->m_res = &res;

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

    types::Vector2 Sprite2d::getTextureSize() const noexcept
    {
        auto sprite = p_handle.get_ref<components::Sprite2d>();
        return sprite->texture_size;
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Sprite2d);
