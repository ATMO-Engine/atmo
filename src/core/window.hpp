#pragma once

#include <SDL3/SDL.h>

namespace atmo
{
    namespace core
    {
        class Window
        {
        protected:
            SDL_Window *sdl_window;
            bool is_main;
        };
    } // namespace core
} // namespace atmo
