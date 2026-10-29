#include "ui_image.hpp"
#include "core/resource/resource_manager.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIImage::RegisterSystems(flecs::world *world)
    {
        world->system<components::UIImage>("UIImage_sync").kind(flecs::PreUpdate).each([](flecs::entity, components::UIImage &image) {
            if (image.texture_path == image.old_texture_path)
                return;

            image.old_texture_path = image.texture_path;
            image.texture = nullptr;
            image.natural_width = 0.0f;
            image.natural_height = 0.0f;

            if (image.texture_path.empty())
                return;

            try {
                image.texture = resource::ResourceManager::GetInstance().getResource<resource::Texture2d>(image.texture_path);
            } catch (const std::exception &e) {
                spdlog::error("UIImage: {}", e.what());
            }
            if (image.texture) {
                image.natural_width = static_cast<float>(image.texture->size().x);
                image.natural_height = static_cast<float>(image.texture->size().y);
            }
        });

        world->observer<components::UIImage>("UIImage_remove").event(flecs::OnRemove).each([](flecs::entity, components::UIImage &image) {
            image.texture = nullptr;
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

        d.userData = &img;
        if (img.raw_texture.isValid()) {
            img.m_drawn_texture = img.raw_texture;
            d.backgroundColor = getComponent<components::UI>().modulate.toFloat<Clay_Color>(255);
            d.image = { .imageData = &img.m_drawn_texture };
            d.layout.sizing.width = { .size = { .minMax = { .min = 0.0f, .max = 0.0f } }, .type = CLAY__SIZING_TYPE_GROW };
            d.layout.sizing.height = { .size = { .minMax = { .min = 0.0f, .max = 0.0f } }, .type = CLAY__SIZING_TYPE_GROW };
            return d;
        }

        img.m_drawn_texture = img.texture ? img.texture->rid() : server::rendering::TextureRid{};
        if (!img.texture)
            return d;

        d.backgroundColor = getComponent<components::UI>().modulate.toFloat<Clay_Color>(255);
        d.image = { .imageData = &img.m_drawn_texture };

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

    void UIImage::draw() {}

    void UIImage::setTexturePath(const std::string &path)
    {
        getComponentMutable<components::UIImage>().texture_path = path;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIImage);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIImage)
