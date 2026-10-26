#pragma once

#include <memory>
#include <string>
#include "SDL3/SDL_rect.h"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/material.hpp"
#include "core/resource/texture_2d.hpp"
#include "meta/meta.hpp"
#include "server/rendering/rid.hpp"
#include "server/rendering/types.hpp"

namespace atmo::core::components
{
    struct Sprite2d {
        std::string texture_path;
        std::string prev_texture_path;
        types::Color modulate{ types::Color::WHITE };
        std::string material_path;
        std::string prev_material_path;
        types::Vector4 custom;
        std::shared_ptr<resource::Texture2d> m_texture;
        std::shared_ptr<resource::Material> m_material;
        server::rendering::CanvasItemRid m_item;
        server::rendering::Affine2d m_transform;
        server::rendering::MaterialRid m_material_rid;
        types::Color m_modulate{ types::Color::WHITE };
        types::Vector4 m_custom;
        bool m_dirty = true;
        types::Vector2 texture_size{ 0.0f, 0.0f };
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Sprite2d> {
    static constexpr const char *name = "Sprite2d";
    static constexpr const char *category = "2D";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::Sprite2d::texture_path>("texture_path").withWidget("file_path"),
        atmo::meta::field<&atmo::core::components::Sprite2d::modulate>("modulate"),
        atmo::meta::field<&atmo::core::components::Sprite2d::material_path>("material_path").withWidget("file_path"),
        atmo::meta::field<&atmo::core::components::Sprite2d::custom>("custom"));
};

namespace atmo::core::ecs::entities
{
    class Sprite2d : public EntityRegistry::Registrable<Sprite2d, Entity2d>
    {
    public:
        using EntityRegistry::Registrable<Sprite2d, Entity2d>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Sprite2d";
        }

        void setTexturePath(const std::string &path);
        std::string_view getTexturePath() const noexcept;

        void setMaterialPath(const std::string &path);
        std::string_view getMaterialPath() const noexcept;
        std::shared_ptr<resource::Material> getMaterial() const;

        void setModulate(const types::Color &modulate);
        types::Color getModulate() const noexcept;

        void setCustom(const types::Vector4 &custom);
        types::Vector4 getCustom() const noexcept;

        types::Vector2 getTextureSize() const noexcept;

        SDL_FRect computeAABB() const override;
    };
} // namespace atmo::core::ecs::entities
