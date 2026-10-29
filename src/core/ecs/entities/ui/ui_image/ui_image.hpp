#pragma once

#include <memory>
#include <string>

#include "core/ecs/entities/ui/ui.hpp"
#include "core/resource/texture_2d.hpp"
#include "server/rendering/rid.hpp"
#include "server/rendering/types.hpp"

namespace atmo::core::components
{
    struct UIImage {
        std::string texture_path;
        std::string old_texture_path;
        std::shared_ptr<resource::Texture2d> texture;
        float natural_width = 0.0f;
        float natural_height = 0.0f;
        server::rendering::TextureRid raw_texture;
        server::rendering::TextureRid m_drawn_texture;
        types::Vector2 rendered_size{ 0.0f, 0.0f };
        float rotation = 0.0f;
        server::rendering::TextureFilter filter = server::rendering::TextureFilter::Linear;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIImage> {
    static constexpr const char *name = "UIImage";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIImage::texture_path>("texture_path").withWidget("file_path"),
        atmo::meta::field<&atmo::core::components::UIImage::rotation>("rotation"));
};

namespace atmo::core::ecs::entities
{
    class UIImage : public EntityRegistry::Registrable<UIImage, UI>
    {
    public:
        using EntityRegistry::Registrable<UIImage, UI>::Registrable;

        static void RegisterSystems(flecs::world *world);
        static void Unregister(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIImage";
        }

        void press();

        Clay_ElementDeclaration buildDecl() override;
        void draw() override;

        void setTexturePath(const std::string &path);
    };
} // namespace atmo::core::ecs::entities
