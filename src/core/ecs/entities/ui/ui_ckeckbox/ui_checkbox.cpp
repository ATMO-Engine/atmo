#include "ui_checkbox.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/events/ui_event/hover_event/hover_event.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UICheckBox::RegisterSystems(flecs::world *world) {}

    void UICheckBox::initialize()
    {
        UIRect::initialize();

        setComponent<components::UICheckBox>({});

        createSignal<UICheckBox &>("toIdle");
        createSignal<UICheckBox &>("Hover");
        createSignal<UICheckBox &>("Clicked");

    }

    Clay_ElementDeclaration UICheckBox::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();

        return d;
    }

    // TODO: WHEN signals done this function might be called by multiple
    //       ui_element so we should move it to somewhere where it will
    //       be shared (and it should only call the signal assigned nothing more)
    void HoverCallBack(Clay_ElementId id, Clay_PointerData data, intptr_t userData)
    {
        int boxId = userData;
        UICheckBox chBox(core::ecs::EntityRegistry::GetEntityFromId(boxId));
        chBox.getSignal<UICheckBox &>("Hover").emit(chBox);

        if (data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
            chBox.getSignal<UICheckBox &>("Clicked").emit(chBox);
        }
    }

    void UICheckBox::draw(ClaySdL3RendererData *data)
    {
        auto &checkComp = getComponentMutable<core::components::UICheckBox>();
        int id = getID();
        Clay_OnHover(HoverCallBack, id);

        if (!Clay_Hovered()) {
            getSignal<UICheckBox &>("ToIdle").emit(*this);
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UICheckBox);
ATMO_REGISTER_COMPONENT(atmo::core::components::UICheckBox)
