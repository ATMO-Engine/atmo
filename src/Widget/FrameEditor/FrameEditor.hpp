#ifndef FrameEditor_HPP_
#define FrameEditor_HPP_

#include <vector>

#include "../Widget.hpp"
#include "SDL3/SDL_render.h"

struct DrawPoint
{
        int x;
        int y;
};

class FrameEditor : public Widget
{
    public:
        FrameEditor(SDL_Window *window);
        ~FrameEditor();

        bool init();
        void run() override;
        void draw();

    private:
        std::vector<DrawPoint> points;
        SDL_Renderer *_renderer;
        SDL_Texture *_texture;
        int _width = 256;
        int _height = 256;
        SDL_Window *_window;
};

#endif /* !FrameEditor_HPP_ */
