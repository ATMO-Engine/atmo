#include "ui_panel.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void UIPanel::RegisterSystems(flecs::world *world) {}

    void UIPanel::initialize()
    {
        UIRect::initialize();
        setComponent<components::UIPanelState>({});

        createSignal<UIPanel &>("Open");
        createSignal<UIPanel &>("Close");
    }

    Clay_ElementDeclaration UIPanel::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();
        // d.floating.attachTo = CLAY_ATTACH_TO_PARENT;
        // d.floating.attachPoints = {
        //     .element = CLAY_ATTACH_POINT_CENTER_CENTER,
        //     .parent = CLAY_ATTACH_POINT_CENTER_CENTER,
        // };
        // d.floating.offset = { 0, 0 };
        // d.floating.zIndex = 10;

        return d;
    }

    void UIPanel::draw(ClaySdL3RendererData *data) {}
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIPanel);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIPanelState)
