#ifndef FrameEditor_HPP_
#define FrameEditor_HPP_

#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "../Widget.hpp"
#include "SDL3/SDL_render.h"

struct DrawPoint {
    int x;
    int y;
};

class FrameEditor : public Widget
{
    public:
        FrameEditor(SDL_Window *window);
        ~FrameEditor();

        void init();
        void run() override;
        void draw();
    private:
        std::vector<DrawPoint> points;
        SDL_Renderer *_renderer;
        SDL_Texture *_texture;
        ImTextureID _textureID;
        int _width = 512;
        int _height = 512;
        SDL_Window *_window;
};

#endif /* !FrameEditor_HPP_ */
