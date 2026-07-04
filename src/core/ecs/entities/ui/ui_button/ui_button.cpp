#include "ui_button.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/ui_event/button_group_toggle_event/button_group_toggle_event.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIButton::RegisterSystems(flecs::world *world)
    {
        world->observer<components::UIButton>("UIButton_remove").event(flecs::OnRemove).each([](flecs::entity, components::UIButton &btn) {
            if (btn.group_event_listener_id != 0)
                event::EventRegistry::RemoveCallBack<event::events::ButtonGroupToggleEvent>(btn.group_event_listener_id);
        });
    }

    void UIButton::Unregister(flecs::world *) {}

    void UIButton::initialize()
    {
        UIRect::initialize();

        setComponent<components::UIButton>({});

        createSignal<>("MouseEntered");
        createSignal<>("MouseExited");
        createSignal<>("Pressed");
        createSignal<>("Released");
        createSignal<bool>("Toggle");

        auto lid = event::EventRegistry::SetCallBack<event::events::ButtonGroupToggleEvent>([ent = this->p_handle](event::events::ButtonGroupToggleEvent *evt) {
            auto self = UIButton(ent);
            if (!self.isAlive())
                return;
            auto &btn = self.getComponentMutable<components::UIButton>();
            auto &rect = self.getComponentMutable<components::UIRect>();
            if (btn.group == 0 || btn.group != evt->group_id)
                return;

            if (static_cast<flecs::entity_t>(self.getID()) == evt->sender_id) {
                btn.is_pressed = true;
                rect.color = types::Color::BLACK;
                rect.color.a = 0.4f;
            } else {
                btn.is_pressed = false;
                rect.color = types::Color::WHITE;
                rect.color.a = 0.0f;
            }
            self.getSignal<bool>("Toggle").emit(btn.is_pressed);
        });
        getComponentMutable<components::UIButton>().group_event_listener_id = lid;

        getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        getComponentMutable<core::components::Layout>().child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        getComponentMutable<core::components::Layout>().child_gap = 4;
        getComponentMutable<core::components::Layout>().padding = { 4, 4, 4, 4 };

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText("Button text");
        label->setFontSize(12);
        label->rename("Button label");
        label->setParent(*this);

        auto &label_comp = label->getComponentMutable<core::components::UILabel>();
        label_comp.text_alignment = core::components::UILabel::TextAlignment::ALIGN_CENTER;
        auto label_layout = label->getComponentMutable<core::components::Layout>();
        label_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        label_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 90.0f, 100.0f };
        label_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        label_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 90.0f, 100.0f };

        auto &label_UI_comp = label->getComponentMutable<core::components::UI>();
        label_UI_comp.modulate = types::Color::BLACK;
    }

    void UIButton::press()
    {
        auto &btnComp = getComponentMutable<components::UIButton>();

        if (!btnComp.toggle) {
            btnComp.is_pressed = true;
            getSignal<>("Pressed").emit();
            return;
        }

        if (btnComp.group != 0) {
            auto evt = std::make_shared<event::events::ButtonGroupToggleEvent>();
            evt->group_id = btnComp.group;
            evt->sender_id = static_cast<flecs::entity_t>(getID());
            event::EventRegistry::Dispatch(evt);
        } else {
            btnComp.is_pressed = !btnComp.is_pressed;
            getSignal<bool>("Toggle").emit(btnComp.is_pressed);
        }
    }

    Clay_ElementDeclaration UIButton::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();

        return d;
    }

    void ButtonHoverCallBack(Clay_ElementId id, Clay_PointerData data, intptr_t userData)
    {
        uint64_t btnId = static_cast<uint64_t>(userData);
        UIButton btn(core::ecs::EntityRegistry::GetEntityFromId(btnId));

        if (!btn.isAlive())
            return;

        auto &btnComp = btn.getComponentMutable<core::components::UIButton>();

        if (!btnComp.toggle) {
            if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
                btnComp.is_pressed = true;
                btn.getSignal<>("Pressed").emit();
            }
            if (data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
                btnComp.is_pressed = false;
                btn.getSignal<>("Released").emit();
            }
        } else {
            if (data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
                if (btnComp.group != 0) {
                    auto evt = std::make_shared<event::events::ButtonGroupToggleEvent>();
                    evt->group_id = btnComp.group;
                    evt->sender_id = static_cast<flecs::entity_t>(btn.getID());
                    event::EventRegistry::Dispatch(evt);
                } else {
                    btnComp.is_pressed = !btnComp.is_pressed;
                    btn.getSignal<bool>("Toggle").emit(btnComp.is_pressed);
                }
            }
        }
    }

    void UIButton::draw(ClaySdL3RendererData *data)
    {
        auto &btnComp = getComponentMutable<core::components::UIButton>();
        intptr_t id = static_cast<intptr_t>(getID());
        Clay_OnHover(ButtonHoverCallBack, id);

        if (btnComp.is_hovered != Clay_Hovered()) {
            btnComp.is_hovered = Clay_Hovered();

            if (!btnComp.is_hovered && btnComp.is_pressed)
                btnComp.is_pressed = false;

            btnComp.is_hovered ? getSignal<>("MouseEntered").emit() : getSignal<>("MouseExited").emit();
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIButton);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIButton)
