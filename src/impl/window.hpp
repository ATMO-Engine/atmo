#pragma once

#include <SDL3/SDL.h>
#include <clay.h>
#include <string>

#include "SDL3_ttf/SDL_ttf.h"
#include "clay_types.hpp"
#include "core/components.hpp"
#include "core/types.hpp"

namespace atmo
{
    namespace impl
    {
        class WindowManager : public core::ComponentManager
        {
        public:
            WindowManager(const atmo::core::components::Window &window);
            ~WindowManager();

            void pollEvents(float deltaTime);
            void draw();

        private:
            SDL_Window *sdl_window = nullptr;
            Clay_SDL3RendererData rendererData;
            Clay_Arena clay_arena;
            bool is_main = false;
        };
    } // namespace impl
} // namespace atmo
#
