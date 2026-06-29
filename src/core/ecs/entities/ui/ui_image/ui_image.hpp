#pragma once

#include "SDL3/SDL_render.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/resource/resource_ref.hpp"

namespace atmo::core::components
{
    struct UIImage {
        std::string texture_path;
        std::string old_texture_path;
        std::unique_ptr<resource::ResourceRef<SDL_Texture>> res;
        float natural_width = 0.0f;
        float natural_height = 0.0f;
        SDL_Texture *raw_texture = nullptr;
        float rendered_size[2] = { 0.f, 0.f };
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIImage> {
    static constexpr const char *name = "UIImage";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::UIImage::texture_path>("texture_path").withWidget("file_path"));
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
        void draw(ClaySdL3RendererData *data) override;
    };
} // namespace atmo::core::ecs::entities
