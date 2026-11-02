#include "sprite_2d.hpp"

#include "core/ecs/components.hpp"
#include "core/ecs/entities/2d/camera_2d/canvas_view.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "core/resource/resource_manager.hpp"
#include "meta/auto_register.hpp"
#include "project/project_manager.hpp"
#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    namespace
    {
        void LoadTexture(components::Sprite2d &sprite)
        {
            sprite.prev_texture_path = sprite.texture_path;
            sprite.m_texture = nullptr;
            sprite.texture_size = { 0.0f, 0.0f };
            sprite.m_dirty = true;

            if (sprite.texture_path.empty())
                return;

            try {
                sprite.m_texture = resource::ResourceManager::GetInstance().getResource<resource::Texture2d>(sprite.texture_path);
            } catch (const std::exception &e) {
                spdlog::error("Failed to open texture for sprite: {}", e.what());
                return;
            }

            if (sprite.m_texture)
                sprite.texture_size = { static_cast<float>(sprite.m_texture->size().x), static_cast<float>(sprite.m_texture->size().y) };
        }
        void LoadMaterial(components::Sprite2d &sprite)
        {
            sprite.prev_material_path = sprite.material_path;
            sprite.m_material = nullptr;
            if (sprite.material_path.empty())
                return;

            try {
                sprite.m_material = resource::ResourceManager::GetInstance().getResource<resource::Material>(sprite.material_path);
            } catch (const std::exception &e) {
                spdlog::error("Failed to open material for sprite: {}", e.what());
                return;
            }
            if (sprite.m_material && sprite.m_material->kind() == server::rendering::ShaderKind::Viewport)
                spdlog::warn("Material {} is a post-processing material; sprites need an atmo::ICanvasMaterial", sprite.material_path);
        }

        bool SameColor(const types::Color &a, const types::Color &b)
        {
            return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
        }

        bool SameVector(const types::Vector4 &a, const types::Vector4 &b)
        {
            return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
        }

        server::rendering::TextureFilter ProjectTextureFilter()
        {
            const std::string &name = project::ProjectManager::GetSettings().rendering.texture_filter;
            if (std::optional<server::rendering::TextureFilter> filter = ParseTextureFilter(name))
                return *filter;

            static bool warned = false;
            if (!warned)
                spdlog::warn("Unknown rendering.texture_filter '{}', expected linear or nearest", name);
            warned = true;
            return server::rendering::TextureFilter::Linear;
        }
    } // namespace

    void Sprite2d::RegisterSystems(flecs::world *world)
    {
        world->system<components::Sprite2d>("Sprite2d_Sync").kind(flecs::PreUpdate).each([](flecs::entity, components::Sprite2d &sprite) {
            if (sprite.texture_path != sprite.prev_texture_path)
                LoadTexture(sprite);
            if (sprite.material_path != sprite.prev_material_path)
                LoadMaterial(sprite);
        });

        world->system<components::Sprite2d, const components::Transform2d>("Sprite2d_Draw")
            .kind(flecs::OnValidate)
            .each([](flecs::entity e, components::Sprite2d &sprite, const components::Transform2d &transform) {
                auto &server = server::rendering::RenderingServer::Instance();

                if (!sprite.m_item.isValid()) {
                    const components::WorldContext *ctx = e.world().try_get<components::WorldContext>();
                    if (!ctx || !ctx->canvas.isValid())
                        return;
                    sprite.m_item = server.canvasItemCreate(ctx->canvas);
                    sprite.m_transform = {};
                    server.canvasItemSetTransform(sprite.m_item, sprite.m_transform);
                    sprite.m_material_rid = {};
                    sprite.m_modulate = types::Color::WHITE;
                    sprite.m_custom = {};
                    sprite.m_dirty = true;
                }

                const server::rendering::MaterialRid material = sprite.m_material ? sprite.m_material->rid() : server::rendering::MaterialRid{};
                if (material != sprite.m_material_rid) {
                    sprite.m_material_rid = material;
                    server.canvasItemSetMaterial(sprite.m_item, material);
                }
                if (!SameColor(sprite.modulate, sprite.m_modulate)) {
                    sprite.m_modulate = sprite.modulate;
                    server.canvasItemSetModulate(sprite.m_item, sprite.modulate);
                }
                if (!SameVector(sprite.custom, sprite.m_custom)) {
                    sprite.m_custom = sprite.custom;
                    server.canvasItemSetCustom(sprite.m_item, sprite.custom);
                }

                if (sprite.m_dirty) {
                    server.canvasItemClear(sprite.m_item);
                    if (sprite.m_texture)
                        server.canvasItemAddTextureRect(
                            sprite.m_item,
                            server::rendering::Rect2::Centered(sprite.texture_size),
                            sprite.m_texture->rid(),
                            { .filter = ProjectTextureFilter() });
                    sprite.m_dirty = false;
                }

                const auto global = server::rendering::Affine2d::FromTrs(transform.g_position, transform.g_rotation, transform.g_scale);
                if (global != sprite.m_transform) {
                    sprite.m_transform = global;
                    server.canvasItemSetTransform(sprite.m_item, global);
                }
            });

        world->observer<components::Sprite2d>("Sprite2d_Remove").event(flecs::OnRemove).each([](flecs::entity, components::Sprite2d &sprite) {
            server::rendering::RenderingServer::Instance().canvasItemFree(sprite.m_item);
            sprite.m_item = {};
            sprite.m_texture = nullptr;
            sprite.m_material = nullptr;
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
        LoadTexture(*sprite.get());

        if (sprite->m_texture)
            spdlog::debug(
                "Loaded Sprite2D texture for entity {}: {} ({}x{})",
                p_handle.name().c_str(),
                sprite->texture_path,
                sprite->texture_size.x,
                sprite->texture_size.y);
    }

    std::string_view Sprite2d::getTexturePath() const noexcept
    {
        auto sprite = p_handle.get_ref<components::Sprite2d>();
        return sprite->texture_path;
    }

    void Sprite2d::setMaterialPath(const std::string &path)
    {
        auto sprite = p_handle.get_ref<components::Sprite2d>();
        sprite->material_path = path;
        LoadMaterial(*sprite.get());
    }

    std::string_view Sprite2d::getMaterialPath() const noexcept
    {
        auto sprite = p_handle.get_ref<components::Sprite2d>();
        return sprite->material_path;
    }

    std::shared_ptr<resource::Material> Sprite2d::getMaterial() const
    {
        auto sprite = p_handle.get_ref<components::Sprite2d>();
        return sprite->m_material;
    }

    void Sprite2d::setModulate(const types::Color &modulate)
    {
        p_handle.get_ref<components::Sprite2d>()->modulate = modulate;
    }

    types::Color Sprite2d::getModulate() const noexcept
    {
        return p_handle.get_ref<components::Sprite2d>()->modulate;
    }

    void Sprite2d::setCustom(const types::Vector4 &custom)
    {
        p_handle.get_ref<components::Sprite2d>()->custom = custom;
    }

    types::Vector4 Sprite2d::getCustom() const noexcept
    {
        return p_handle.get_ref<components::Sprite2d>()->custom;
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
