#pragma once

#include <string_view>
#include "core/event/events/ui_event/ui_event.hpp"


namespace atmo::core::event::events
{

    class HoverEvent : public event::EventRegistry::Registrable<HoverEvent, UIEvent>
    {
    public:
        using event::EventRegistry::Registrable<HoverEvent, UIEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "HoverEvent";
        }
    };
} // namespace atmo::core::event::events
