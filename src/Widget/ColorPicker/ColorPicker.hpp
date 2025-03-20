#ifndef ColorPicker_HPP_
#define ColorPicker_HPP_

#include "../Widget.hpp"
#include "imgui.h"

class ColorPicker : public Widget
{
    public:
        ColorPicker();
        ~ColorPicker();

        enum class Tool
        {
            PENCIL,
            ERASER,
            BUCKET
        };

        float getSize() { return _size; }
        float *getColor() { return _currentColor; }
        bool getEraser() { return _eraser; }
        bool getClear() { return _clear; }

        void run() override;
        void init(ColorPicker::Tool *selectedTool);

    private:
        float _currentColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        Tool *_selectedTool;
        float _size = 2.0f;
        bool _eraser = false;
        bool _clear = false;
};

#endif /* !ColorPicker_HPP_ */
