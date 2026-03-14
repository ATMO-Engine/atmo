#pragma once

#include <string_view>
#include "core/event/events/ui_event/ui_event.hpp"


namespace atmo::core::event::events
{

    /**
     * @brief Event representing a hover action on a UI element. This event is triggered when the mouse cursor enters the bounds of a UI element, and can be
     * used to implement hover effects, tooltips, etc.
     *
     */
    class HoverEvent : public event::EventRegistry::Registrable<HoverEvent, UIEvent>
    {
    public:
        using event::EventRegistry::Registrable<HoverEvent, UIEvent>::Registrable;

        /**
         * @brief Get the Local Name of the event, used for registration and dispatching
         *
         * @return constexpr std::string_view
         */
        static constexpr std::string_view LocalName()
        {
            return "HoverEvent";
        }
    };
} // namespace atmo::core::event::events
