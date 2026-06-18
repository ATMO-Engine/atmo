#include "ui_button.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entity_registry.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIButton::RegisterSystems(flecs::world *world) {}

    void UIButton::initialize()
    {
        UIRect::initialize();

        setComponent<components::UIButton>({});

        createSignal<>("MouseEntered");
        createSignal<>("MouseExited");
        createSignal<>("Pressed");
        createSignal<>("Released");

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
    }

    Clay_ElementDeclaration UIButton::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();

        return d;
    }

    void ButtonHoverCallBack(Clay_ElementId id, Clay_PointerData data, intptr_t userData)
    {
        int btnId = userData;
        UIButton btn(core::ecs::EntityRegistry::GetEntityFromId(btnId));
        auto &btnComp = btn.getComponentMutable<core::components::UIButton>();

        if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
            btnComp.is_pressed = true;
            btn.getSignal<>("Pressed").emit();
        }
        if (data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
            btnComp.is_pressed = false;
            btn.getSignal<>("Released").emit();
        }
    }

    void UIButton::draw(ClaySdL3RendererData *data)
    {
        auto &btnComp = getComponentMutable<core::components::UIButton>();
        int id = getID();
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
