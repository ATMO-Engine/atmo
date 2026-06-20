#pragma once

#include <flecs.h>
#include <string_view>
#include "core/event/events/ui_event/ui_event.hpp"

namespace atmo::core::event::events
{
    class ButtonGroupToggleEvent : public event::EventRegistry::Registrable<ButtonGroupToggleEvent, UIEvent>
    {
    public:
        using event::EventRegistry::Registrable<ButtonGroupToggleEvent, UIEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "ButtonGroupToggleEvent";
        }

        int group_id = 0;
        flecs::entity_t sender_id = 0;
    };
} // namespace atmo::core::event::events
