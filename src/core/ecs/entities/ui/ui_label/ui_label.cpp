#include "ui_label.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "core/types.hpp"
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

    void UILabel::draw()
    {
        auto &ui = getComponent<components::UI>();
        auto &label = getComponent<components::UILabel>();

        // uint16_t fontId,
        // uint16_t fontSize,

        // Clay_TextElementConfigWrapMode wrapMode,
        // Clay_TextAlignment textAlignment

        Clay_TextElementConfig conf = { .textColor = (types::Color::WHITE * ui.modulate).toFloat<Clay_Color>(255),
                                        .letterSpacing = label.letter_spacing,
                                        .lineHeight = label.line_height };

        // CLAY_STRING(string)
        // CLAY_TEXT(label.text.c_str(), conf);
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UILabel);
ATMO_REGISTER_COMPONENT(atmo::core::components::UILabel)
