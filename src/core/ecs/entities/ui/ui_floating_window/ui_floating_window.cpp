#include "ui_floating_window.hpp"
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

        return d;
    }

    void UIFloatingWindow::draw(ClaySdL3RendererData *data) {}
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFloatingWindow);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIFloatingWindowState)
