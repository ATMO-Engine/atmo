#include "ui_panel.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void UIFloatingWindow::RegisterSystems(flecs::world *world) {}

    void UIFloatingWindow::initialize()
    {
        UIRect::initialize();
        setComponent<components::UIFloatingWindowState>({});

        createSignal<UIFloatingWindow &>("Open");
        createSignal<UIFloatingWindow &>("Close");
    }

    Clay_ElementDeclaration UIFloatingWindow::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();
        d.floating.attachTo = CLAY_ATTACH_TO_PARENT;
        d.floating.attachPoints = {
            .element = CLAY_ATTACH_POINT_LEFT_CENTER,
            .parent = CLAY_ATTACH_POINT_RIGHT_CENTER,
        };
        d.floating.offset = { 0, 0 };
        d.floating.zIndex = 10;

        return d;
    }

    void UIFloatingWindow::draw(ClaySdL3RendererData *data) {}
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFloatingWindow);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIFloatingWindowState)
