#include "ui_popup.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIPopup::RegisterSystems(flecs::world *world) {}

    void UIPopup::initialize()
    {
        UIRect::initialize();

        getComponentMutable<components::UIRect>().color = types::Color("#8080805f");
        spdlog::info("popup bg color: {}", getComponentMutable<components::UIRect>().color);

        auto &layout = getComponentMutable<components::Layout>();
        layout.child_alignment.horizontal = components::Layout::ChildAlignment::Center;
        layout.child_alignment.vertical = components::Layout::ChildAlignment::Center;
    }

    Clay_ElementDeclaration UIPopup::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();

        d.floating.attachTo = CLAY_ATTACH_TO_ROOT;
        d.layout.sizing.width.type = CLAY__SIZING_TYPE_GROW;
        d.layout.sizing.height.type = CLAY__SIZING_TYPE_GROW;

        return d;
    }

    void UIPopup::draw(ClaySdL3RendererData *data) {}
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIPopup);
