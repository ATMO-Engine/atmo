#include "ui_label.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void UILabel::RegisterSystems(flecs::world *world)
    {
        world->observer<components::UILabel>("UILabel_remove").event(flecs::OnRemove).each([](flecs::entity e, components::UILabel &sprite) {
            if (sprite.font_path.empty())
                return;

            resource::ResourceRef<TTF_Font> res = resource::ResourceManager::GetInstance().getResource<TTF_Font>(sprite.font_handle.assetId);

            res.unpin();
        });
    }

    void UILabel::initialize()
    {
        UI::initialize();

        setComponent<components::UILabel>({});
    }

    void UILabel::setFontPath(const std::string &path)
    {
        auto label = p_handle.get_ref<components::UILabel>();
        label->font_path = path;

        if (label->font_path.empty())
            return;

        label->font_handle = resource::Handle<TTF_Font>{ .assetId = label->font_path };

        resource::ResourceRef<TTF_Font> res = resource::ResourceManager::GetInstance().getResource<TTF_Font>(label->font_handle.assetId);

        res.pin();
    }

    std::string_view UILabel::getFontPath() const noexcept
    {
        auto label = p_handle.get_ref<components::UILabel>();
        return label->font_path;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UILabel);
ATMO_REGISTER_COMPONENT(atmo::core::components::UILabel)
