#pragma once

#include "SDL3/SDL_events.h"
#include "core/event/Aevent.hpp"

namespace atmo
{
    namespace core
    {
        namespace impl
        {
            /**
             * @brief SDL-specific event class that wraps an SDL_Event.
             */
            class SDLEvent : public event::AEvent
            {
            public:
                /**
                 * @brief Constructs an SDLEvent from an SDL_Event.
                 *
                 * @param sdlEvent The SDL_Event to wrap.
                 */
                SDLEvent(const SDL_Event sdlEvent);
                /**
                 * @brief Virtual destructor for SDLEvent.
                 */
                ~SDLEvent() override = default;
                /**
                 * @brief The underlying SDL_Event.
                 */
                SDL_Event sdl_event;
                /**
                 * @brief Time delta associated with the event.
                 */
                float delta = 0.0f;

                /**
                 * @brief Retrieves the underlying SDL_Event.
                 *
                 * @return A constant reference to the SDL_Event.
                 */
                const SDL_Event &getSDLEvent() const;
            };
        } // namespace impl
    } // namespace core
} // namespace atmo
