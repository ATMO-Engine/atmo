#ifndef FrameEditor_HPP_
#define FrameEditor_HPP_

#include <map>
#include <utility>
#include <vector>

#include "../Widget.hpp"

struct Point
{
        int x;
        int y;
};

class FrameEditor : public Widget
{
    public:
        class DrawingContext
        {
            public:
                DrawingContext(unsigned char r, unsigned char g, unsigned char b, unsigned char a, float thickness)
                {
                    _r = r;
                    _g = g;
                    _b = b;
                    _a = a;
                    _thickn = thickness;
                }

                int getColor()
                {
                    int col = 0;
                    col |= _r << 24;
                    col |= _g << 16;
                    col |= _b << 8;
                    col |= _a << 0;
                    return col;
                }

                float getThickness() { return _thickn; }

                unsigned char _r = 0;
                unsigned char _g = 0;
                unsigned char _b = 0;
                unsigned char _a = 255;
                float _thickn = 2;
        };

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
                _r = 255;
                _g = 255;
                _b = 255;
                _a = 255;
            }
        }

        FrameEditor();
        ~FrameEditor();

        bool init();
        void run() override;
        void draw();

    private:
        std::pair<DrawingContext, std::vector<Point>> _drawnList; // Store drawn points
        std::vector<std::pair<DrawingContext, std::vector<Point>>> _drawnPoints; // Store drawn points

        unsigned char _r = 0;
        unsigned char _g = 0;
        unsigned char _b = 0;
        unsigned char _a = 255;

        float _thickness = 2;
};

#endif /* !FrameEditor_HPP_ */
