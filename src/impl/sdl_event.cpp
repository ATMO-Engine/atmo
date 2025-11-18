#include "sdl_event.hpp"

namespace atmo
{
    namespace core
    {
        namespace impl
        {
            SDLEvent::SDLEvent(const SDL_Event sdlEvent) : sdlEvent(sdlEvent) {}

            const SDL_Event &SDLEvent::getSDLEvent() const
            {
                return sdlEvent;
            }
        } // namespace impl
    } // namespace core
} // namespace atmo
