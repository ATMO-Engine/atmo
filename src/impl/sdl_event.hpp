#pragma once

#include "SDL3/SDL_events.h"
#include "core/event/Ievent.hpp"

namespace atmo
{
    namespace core
    {
        namespace impl
        {
            class SDLEvent : public event::IEvent
            {
            public:
                SDLEvent(const SDL_Event sdlEvent);
                ~SDLEvent() override = default;
                SDL_Event sdlEvent;
                float delta = 0.0f;

                const SDL_Event &getSDLEvent() const;
            };
        } // namespace impl
    } // namespace core
} // namespace atmo
