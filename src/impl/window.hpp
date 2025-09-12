#pragma once

#include <SDL3/SDL.h>
#include <clay.h>
#include <string>

#include "core/components.hpp"
#include "core/types.hpp"

namespace atmo
{
    namespace impl
    {
        class WindowManager : public core::ComponentManager
        {
        public:
            WindowManager(atmo::core::components::Window &window);
            ~WindowManager();

            void draw();

        private:
            SDL_Window *sdl_window = nullptr;
            SDL_Renderer *sdl_renderer = nullptr;
            Clay_Arena clay_arena;
            bool is_main = false;
        };
    } // namespace impl
} // namespace atmo
#
