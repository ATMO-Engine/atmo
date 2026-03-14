#pragma once
#include "core/event/events/sdl_event/sdl_event.hpp"

namespace atmo::core::event::events
{
    /**
     * @brief Event representing any input event (keyboard, mouse, gamepad, etc.)
     *
     */
    class InputEvent : public event::EventRegistry::Registrable<InputEvent, event::events::SDLEvent>
    {
    public:
        using event::EventRegistry::Registrable<InputEvent, event::events::SDLEvent>::Registrable;

        /**
         * @brief Get the Local Name of the event, used for registration and dispatching
         *
         * @return constexpr std::string_view
         */
        static constexpr std::string_view LocalName()
        {
            return "InputEvent";
        }
    };
} // namespace atmo::core::event::events
