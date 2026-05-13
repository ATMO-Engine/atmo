#include "ui_rect.hpp"
#include <iostream>
#include "clay.h"
#include "core/event/events/ui_event/hover_event/hover_event.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIRect::RegisterSystems(flecs::world *world) {}

    void UIRect::initialize()
    {
        UI::initialize();

        setComponent<components::UIRect>({});
    }

    Clay_ElementDeclaration UIRect::buildDecl()
    {
        Clay_ElementDeclaration d = UI::buildDecl();

        const auto &ui = getComponentMutable<components::UI>();
        const auto &rect = getComponentMutable<components::UIRect>();

        d.border.color = (rect.border.color * ui.modulate).toFloat<Clay_Color>(255);

        d.border.width = { rect.border.left, rect.border.right, rect.border.top, rect.border.bottom, rect.border.between_children };

        d.backgroundColor = (rect.color * ui.modulate).toFloat<Clay_Color>(255);
        d.cornerRadius = { rect.corner_radius.top_left, rect.corner_radius.top_right, rect.corner_radius.bottom_left, rect.corner_radius.bottom_right };

        event::EventRegistry::SetCallBack<event::events::HoverEvent>(
            [ui](event::events::HoverEvent *event) { std::cout << "UI element ID from event: " << ui.element_id.id << std::endl; });
        return d;
    }

    void HoverCallBack(Clay_ElementId id, Clay_PointerData data, intptr_t userData)
    {
        auto *rect = reinterpret_cast<components::UIRect *>(userData);

        if (data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
            rect->color = types::Color{ static_cast<uint8_t>(255), static_cast<uint8_t>(100), static_cast<uint8_t>(0), static_cast<uint8_t>(255) };
        }

        auto default_event = atmo::core::event::EventRegistry::Create<atmo::core::event::events::HoverEvent>("Event::UIEvent::HoverEvent");
        atmo::core::event::EventRegistry::Dispatch(default_event);
    }

    void UIRect::draw(ClaySdL3RendererData *data)
    {
        auto &rect = getComponentMutable<components::UIRect>();

        Clay_OnHover(HoverCallBack, reinterpret_cast<intptr_t>(&rect));
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIRect);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIRect)
