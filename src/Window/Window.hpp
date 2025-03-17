#ifndef WINDOW_HPP_
#define WINDOW_HPP_

#include <SDL3/SDL.h>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include "../Editor/SceneEditor/SceneEditor.hpp"
#include "../Editor/SpriteEditor/SpriteEditor.hpp"
#include "SDL3/SDL_video.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_opengl3_loader.h"
#include "imgui_impl_sdl3.h"

class Window
{
    public:
        Window();
        ~Window();
        bool init();
        void setupImGui();
        void run();
        void destroy();

    private:
        SDL_Window *window;
        bool shouldClose;
        SDL_GLContext context;
        SceneEditor *sceneEditor;
        SpriteEditor *textureEditor;
};

#endif /* !WINDOW_HPP_ */
