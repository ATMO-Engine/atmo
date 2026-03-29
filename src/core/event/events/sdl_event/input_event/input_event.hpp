#pragma once
#include "core/event/events/sdl_event/sdl_event.hpp"

namespace atmo::core::event::events
{

    class InputEvent : public event::EventRegistry::Registrable<InputEvent, event::events::SDLEvent>
    {
    public:
        using event::EventRegistry::Registrable<InputEvent, event::events::SDLEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "InputEvent";
        }
    };
} // namespace atmo::core::event::events
