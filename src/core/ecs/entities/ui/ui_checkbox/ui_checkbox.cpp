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
        auto &chBoxComp = getComponentMutable<core::components::UICheckBox>();
        chBoxComp.trigger = false;

        auto &rect = getComponentMutable<core::components::UIRect>();
        rect.border.color = core::types::Color{ static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(100) };
        rect.border.left = 5;
        rect.border.top = 5;
        rect.border.bottom = 5;
        rect.border.right = 5;

        rect.corner_radius.top_left = 5.0f;
        rect.corner_radius.top_right = 5.0f;
        rect.corner_radius.bottom_left = 5.0f;
        rect.corner_radius.bottom_right = 5.0f;


        createSignal<UICheckBox &>("ToIdle");
        getSignal<core::ecs::entities::UICheckBox &>("ToIdle").connect([](core::ecs::entities::UICheckBox &chBox) {
            auto &chBoxComp = chBox.getComponentMutable<core::components::UICheckBox>();
            chBoxComp.state = core::components::UICheckBox::CheckBoxState::IDLE;

            auto &rect = chBox.getComponentMutable<core::components::UIRect>();
            rect.border.color = core::types::Color{ static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(100) };

            if (chBoxComp.trigger == true) {
                rect.color = core::types::Color::WHITE;
            } else {
                rect.color = core::types::Color::BLACK;
            }
        });

        createSignal<UICheckBox &>("Hover");
        getSignal<core::ecs::entities::UICheckBox &>("Hover").connect([](core::ecs::entities::UICheckBox &chBox) {
            auto &chBoxComp = chBox.getComponentMutable<core::components::UICheckBox>();
            chBoxComp.state = core::components::UICheckBox::CheckBoxState::HOVER;

            auto &rect = chBox.getComponentMutable<core::components::UIRect>();
            rect.border.color = core::types::Color::BLACK;
        });

        createSignal<UICheckBox &>("Clicked");
        getSignal<core::ecs::entities::UICheckBox &>("Clicked").connect([](core::ecs::entities::UICheckBox &chBox) {
            auto &chBoxComp = chBox.getComponentMutable<core::components::UICheckBox>();
            chBoxComp.trigger = !chBoxComp.trigger;

            auto &rect = chBox.getComponentMutable<core::components::UIRect>();
            if (chBoxComp.trigger == true) {
                rect.color = core::types::Color::WHITE;
            } else {
                rect.color = core::types::Color::BLACK;
            }
        });
    }

    Clay_ElementDeclaration UICheckBox::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();

        return d;
    }

    // TODO: WHEN signals done this function might be called by multiple
    //       ui_element so we should move it to somewhere where it will
    //       be shared (and it should only call the signal assigned nothing more)
    void ChecBoxHoverCallBack(Clay_ElementId id, Clay_PointerData data, intptr_t userData)
    {
        uint64_t boxId = static_cast<uint64_t>(userData);
        UICheckBox chBox(core::ecs::EntityRegistry::GetEntityFromId(boxId));
        if (!chBox.isAlive())
            return;
        chBox.getSignal<UICheckBox &>("Hover").emit(chBox);

        if (data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
            chBox.getSignal<UICheckBox &>("Clicked").emit(chBox);
        }
    }

    void UICheckBox::draw(ClaySdL3RendererData *data)
    {
        auto &checkComp = getComponentMutable<core::components::UICheckBox>();
        intptr_t id = static_cast<intptr_t>(getID());
        Clay_OnHover(ChecBoxHoverCallBack, id);

        if (!Clay_Hovered() && checkComp.state != core::components::UICheckBox::CheckBoxState::IDLE) {
            getSignal<UICheckBox &>("ToIdle").emit(*this);
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UICheckBox);
ATMO_REGISTER_COMPONENT(atmo::core::components::UICheckBox)
