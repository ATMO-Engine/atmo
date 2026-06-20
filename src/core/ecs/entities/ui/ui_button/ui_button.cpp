#include "ui_button.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "flecs/addons/cpp/mixins/query/impl.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    flecs::query<components::UIButton, components::UIRect> *UIButton::m_query = nullptr;

    void UIButton::RegisterSystems(flecs::world *world)
    {
        m_query = new flecs::query<components::UIButton, components::UIRect>(world->query<components::UIButton &, components::UIRect &>());
    }

    void UIButton::Unregister(flecs::world *world)
    {
        delete m_query;
        m_query = nullptr;
    }

    void UIButton::initialize()
    {
        UIRect::initialize();

        setComponent<components::UIButton>({});

        createSignal<>("MouseEntered");
        createSignal<>("MouseExited");
        createSignal<>("Pressed");
        createSignal<>("Released");
        createSignal<int>("Toggle");

        getSignal<int>("Toggle").connect([ent = this->p_handle](int group_id) {
            auto ent_button = UIButton(ent);
            ent_button.m_query->each([group_id](components::UIButton &btn, components::UIRect &rect) {
                if (btn.group == group_id) {
                    btn.is_pressed = false;
                    rect.color = types::Color::WHITE;
                    rect.color.a = 0.0f;
                }
            });

            ent_button.getComponentMutable<core::components::UIButton>().is_pressed = true;
            ent_button.getComponentMutable<core::components::UIRect>().color = types::Color::BLACK;
            ent_button.getComponentMutable<core::components::UIRect>().color.a = 0.4f;
        });

        getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        getComponentMutable<core::components::Layout>().child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
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
                btnComp.is_pressed = !btnComp.is_pressed;
                btn.getSignal<int>("Toggle").emit(btnComp.group);
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
