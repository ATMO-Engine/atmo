#ifndef FrameEditor_HPP_
#define FrameEditor_HPP_

#include <queue>
#include <utility>
#include <vector>

#include "../Widget.hpp"
#include "SDL3/SDL_render.h"
#include "imgui.h"

struct Point
{
        int x;
        int y;
};

class FrameEditor : public Widget
{
    public:
        void setColor(float *color)
        {
            _r = color[0] * 255;
            _g = color[1] * 255;
            _b = color[2] * 255;
            _a = color[3] * 255;
        }

        void setThickness(float thick) { _thickness = thick; }

        void setEraser(bool eraser)
        {
            if (eraser) {
                _r = 0;
                _g = 0;
                _b = 0;
                _a = 0;
            }
        }

        FrameEditor(SDL_Renderer *renderer);
        ~FrameEditor();

        bool init(int width, int heigth);
        void run() override;
        void draw();
        void drawLine(int x, int y);
        void drawPoint(int x, int y);
        void clear();

    private:
        SDL_Renderer *_renderer = nullptr;
        SDL_Texture *_texture = nullptr;
        SDL_Texture *_background = nullptr;

        ImVec2 _beginPos;
        ImVec2 _size;

        ImVec2 _lastPos;

        unsigned char _r = 0;
        unsigned char _g = 0;
        unsigned char _b = 0;
        unsigned char _a = 255;

        int _width = 0;
        int _heigth = 0;

        float _thickness = 2;
};

#endif /* !FrameEditor_HPP_ */
