#include "ui_button.hpp"
#include "clay.h"
#include <iostream>
#include "core/event/events/ui_event/hover_event/hover_event.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIButton::RegisterSystems(flecs::world *world) {}

    void UIButton::initialize()
    {
        UIRect::initialize();

        //TODO: signals sytem to handle internal events
        const auto &ui = getComponentMutable<components::UI>();
        event::EventRegistry::SetCallBack<event::events::HoverEvent>(
            [ui](event::events::HoverEvent *event) {
                std::cout << "UI element ID from event: " << ui.element_id.id << std::endl;
            });

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText("Button text");
        label->setFontSize(32);
        label->rename("Button label");
        label->setParent(*this);


        auto& label_comp = label->getComponentMutable<core::components::UILabel>();
        label_comp.text_alignment = core::components::UILabel::TextAlignment::ALIGN_CENTER;
    }

    Clay_ElementDeclaration UIButton::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();

        auto label = getChild("Button label");
        if (label.hasComponent<core::components::UILabel>()) {
            auto& label_layout = label.getComponentMutable<core::components::Layout>();
            auto &selfLayout = getComponentMutable<core::components::Layout>();

            auto halfSize = [](const auto& sizeVariant) -> float {
                return std::visit([](auto&& size) -> float {
                    using T = std::decay_t<decltype(size)>;
                    if constexpr (std::is_same_v<T, float>) {
                        return size / 2;
                    } else {
                        return size.max / 2;
                    }
                }, sizeVariant);
            };

            label_layout.padding.left = halfSize(selfLayout.width.size);
            label_layout.padding.top  = halfSize(selfLayout.height.size);
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
        auto *rect = reinterpret_cast<components::UIRect *>(userData);

        if (data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
            rect->color = types::Color{ static_cast<uint8_t>(255), static_cast<uint8_t>(100), static_cast<uint8_t>(0), static_cast<uint8_t>(255) };
        }

        auto default_event = atmo::core::event::EventRegistry::Create<atmo::core::event::events::HoverEvent>("Event::UIEvent::HoverEvent");
        atmo::core::event::EventRegistry::Dispatch(default_event);
    }

    void UIButton::draw(ClaySdL3RendererData *data)
    {
        auto &rect = getComponentMutable<components::UIRect>();

        Clay_OnHover(HoverCallBack, reinterpret_cast<intptr_t>(&rect));
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIButton);
