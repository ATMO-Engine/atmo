#pragma once

#include "SDL3/SDL_events.h"
#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"

namespace atmo::core::event::events
{

    class SDLEvent : public event::EventRegistry::Registrable<SDLEvent, Event>
    {
    public:
        using event::EventRegistry::Registrable<SDLEvent, Event>::Registrable;

        /**
         * @brief The actual SDL event data, which can be used to get more information about the event, such as the type of event, the window it occurred on,
         * etc.
         *
         */
        SDL_Event sdl_event = {};

        /**
         * @brief The time in seconds since the last SDL event.
         *
         */
        float delta = 0.0f;

        static constexpr std::string_view LocalName()
        {
            return "SDLEvent";
        }
    };
} // namespace atmo::core::event::events
