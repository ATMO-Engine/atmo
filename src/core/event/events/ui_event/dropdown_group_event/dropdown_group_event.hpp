#pragma once

#include <flecs.h>
#include <string_view>
#include "core/event/events/ui_event/ui_event.hpp"

namespace atmo::core::event::events
{
    class DropdownGroupEvent : public event::EventRegistry::Registrable<DropdownGroupEvent, UIEvent>
    {
    public:
        using event::EventRegistry::Registrable<DropdownGroupEvent, UIEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "DropdownGroupEvent";
        }

        enum class Action {
            Hovered,
            Closed
        };

        Action action = Action::Hovered;
        int group_id = 0;
        flecs::entity_t sender_id = 0;
        int sender_depth = 0;
    };
} // namespace atmo::core::event::events
