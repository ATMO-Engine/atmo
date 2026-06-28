#include "ui_image.hpp"
#include "SDL3/SDL_render.h"
#include "core/resource/resource_manager.hpp"

namespace atmo::core::ecs::entities
{
    void UIImage::RegisterSystems(flecs::world *world)
    {
        world->system<components::UIImage>("UIImage_sync").kind(flecs::PreUpdate).each([](flecs::entity /*e*/, components::UIImage &image) {
            if (image.texture_path == image.old_texture_path)
                return;

            image.res = atmo::core::resource::ResourceManager::GetInstance().getResource<SDL_Texture>(image.texture_path);
            image.old_texture_path = image.texture_path;
            auto tex = image.res->get();
            if (tex)
                SDL_GetTextureSize(tex.get(), &image.natural_width, &image.natural_height);
        });
    }

    void UIImage::Unregister(flecs::world *) {}

    void UIImage::initialize()
    {
        UI::initialize();

        setComponent<components::UIImage>({});
    }

    Clay_ElementDeclaration UIImage::buildDecl()
    {
        Clay_ElementDeclaration d = UI::buildDecl();

        auto &img = getComponentMutable<components::UIImage>();

        if (img.raw_texture) {
            d.backgroundColor = getComponent<components::UI>().modulate.toFloat<Clay_Color>(255);
            d.image = { .imageData = img.raw_texture };
            d.layout.sizing.width = { .size = { .minMax = { .min = 0.0f, .max = 0.0f } }, .type = CLAY__SIZING_TYPE_GROW };
            d.layout.sizing.height = { .size = { .minMax = { .min = 0.0f, .max = 0.0f } }, .type = CLAY__SIZING_TYPE_GROW };
            d.userData = &img.rendered_size[0];
            return d;
        }

        if (!img.res)
            return d;

        auto tex = img.res->get();
        if (!tex)
            return d;

        d.backgroundColor = getComponent<components::UI>().modulate.toFloat<Clay_Color>(255);
        d.image = { .imageData = tex.get() };

        auto &w = d.layout.sizing.width;
        auto &h = d.layout.sizing.height;
        if (w.type == CLAY__SIZING_TYPE_FIT && w.size.minMax.max == 0.0f) {
            w = { .size = { .minMax = { .min = 0.0f, .max = 0.0f } }, .type = CLAY__SIZING_TYPE_GROW };
            h = { .size = { .minMax = { .min = 0.0f, .max = 0.0f } }, .type = CLAY__SIZING_TYPE_GROW };
        }

        if (d.aspectRatio.aspectRatio == 0.0f && img.natural_width > 0.0f && img.natural_height > 0.0f)
            d.aspectRatio = { .aspectRatio = img.natural_width / img.natural_height };

        return d;
    }

    void UIImage::draw(ClaySdL3RendererData *data) {}
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIImage);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIImage)
