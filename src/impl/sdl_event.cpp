#include "sdl_event.hpp"

namespace atmo
{
    namespace core
    {
        namespace impl
        {
            SDLEvent::SDLEvent(const SDL_Event sdlEvent) : sdl_event(sdlEvent) {}

            const SDL_Event &SDLEvent::getSDLEvent() const
            {
                return sdl_event;
            }
        } // namespace impl
    } // namespace core
} // namespace atmo
