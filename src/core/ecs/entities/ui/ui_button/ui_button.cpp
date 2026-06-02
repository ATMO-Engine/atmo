#include "ui_button.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/events/ui_event/hover_event/hover_event.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIButton::RegisterSystems(flecs::world *world) {}

    void UIButton::initialize()
    {
        UIRect::initialize();

        setComponent<components::UIButton>({});

        createSignal<UIButton &>("ToIdle");
        createSignal<UIButton &>("Hover");
        createSignal<UIButton &>("Pressed");
        createSignal<UIButton &>("Released");


        const auto &ui = getComponentMutable<components::UI>();
        event::EventRegistry::SetCallBack<event::events::HoverEvent>(
            [ui](event::events::HoverEvent *event) { std::cout << "UI element ID from event: " << ui.element_id.id << std::endl; });

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText("Button text");
        label->setFontSize(32);
        label->rename("Button label");
        label->setParent(*this);

        auto &label_comp = label->getComponentMutable<core::components::UILabel>();
        label_comp.text_alignment = core::components::UILabel::TextAlignment::ALIGN_CENTER;
    }

    Clay_ElementDeclaration UIButton::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();

        auto label = getChild("Button label");
        if (label.hasComponent<core::components::UILabel>()) {
            auto &label_layout = label.getComponentMutable<core::components::Layout>();
            auto &selfLayout = getComponentMutable<core::components::Layout>();

            auto getSize = [](const auto &sizeVariant) -> float {
                return std::visit(
                    [](auto &&size) -> float {
                        using T = std::decay_t<decltype(size)>;
                        if constexpr (std::is_same_v<T, float>) {
                            return size;
                        } else {
                            return size.max;
                        }
                    },
                    sizeVariant);
            };

            label_layout.width.size = selfLayout.width.size;
            label_layout.height.size = selfLayout.height.size;
        } else {
            spdlog::warn("Button label missing UILabel component");
        }

        return d;
    }

    // TODO: WHEN signals done this function might be called by multiple
    //       ui_element so we should move it to somewhere where it will
    //       be shared (and it should only call the signal assigned nothing more)
    void HoverCallBack(Clay_ElementId id, Clay_PointerData data, intptr_t userData)
    {
        int btnId = userData;
        UIButton btn(core::ecs::EntityRegistry::GetEntityFromId(btnId));
        btn.getSignal<UIButton &>("Hover").emit(btn);

        if (data.state == CLAY_POINTER_DATA_PRESSED) {
            btn.getSignal<UIButton &>("Pressed").emit(btn);
        }
        if (data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
            btn.getSignal<UIButton &>("Released").emit(btn);
        }
    }

    void UIButton::draw(ClaySdL3RendererData *data)
    {
        auto &btnComp = getComponentMutable<core::components::UIButton>();
        int id = getID();
        Clay_OnHover(HoverCallBack, id);

        if (!Clay_Hovered() && btnComp.state != core::components::UIButton::ButtonState::IDLE) {
            getSignal<UIButton &>("ToIdle").emit(*this);
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIButton);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIButton)
