#include "ui_rect.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIRect::RegisterSystems(flecs::world *world) {}

    void UIRect::initialize()
    {
        UI::initialize();

        setComponent<components::UIRect>({});
    }

    Clay_ElementDeclaration UIRect::buildDecl()
    {
        Clay_ElementDeclaration d = UI::buildDecl();

        const auto &ui = getComponentMutable<components::UI>();
        const auto &rect = getComponentMutable<components::UIRect>();

        d.border.color = (rect.border.color * ui.modulate).toFloat<Clay_Color>(255);

        d.border.width = { rect.border.left, rect.border.right, rect.border.top, rect.border.bottom, rect.border.between_children };

        d.backgroundColor = (rect.color * ui.modulate).toFloat<Clay_Color>(255);
        d.cornerRadius = { rect.corner_radius.top_left, rect.corner_radius.top_right, rect.corner_radius.bottom_left, rect.corner_radius.bottom_right };

        return d;
    }

    void UIRect::draw(ClaySdL3RendererData *data) {}
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIRect);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIRect)
