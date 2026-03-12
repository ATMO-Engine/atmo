#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"

namespace atmo::core::event::events
{
    /**
     * @brief Event representing a generic UI event, which can be used as a base class for more specific UI events such as hover, click, etc. This event can be
     * used to listen to all UI events in a generic way, or to implement functionality that should happen on all UI events regardless of their specific type.
     *
     */
    class UIEvent : public event::EventRegistry::Registrable<UIEvent, Event>
    {
    public:
        using event::EventRegistry::Registrable<UIEvent, Event>::Registrable;

        /**
         * @brief Get the Local Name of the event, used for registration and dispatching
         *
         * @return constexpr std::string_view
         */
        static constexpr std::string_view LocalName()
        {
            return "UIEvent";
        }

    private:
        /**
         * @brief The entity ID of the UI element that the event occurred on.
         *
         */
        flecs::entity m_entity_id;
    };

} // namespace atmo::core::event::events
