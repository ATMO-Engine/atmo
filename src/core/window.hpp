#pragma once

#include <SDL3/SDL.h>
#include "core/scene.hpp"
#include "core/types.hpp"

namespace atmo
{
    namespace core
    {
        class Window
        {
        protected:
            SDL_Window *sdl_window;
            std::vector<Scene *> singletons;
            Scene scene;
            bool is_main;
        };
    } // namespace core
} // namespace atmo
