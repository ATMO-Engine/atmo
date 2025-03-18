#ifndef ColorPicker_HPP_
#define ColorPicker_HPP_

#include "../Widget.hpp"
#include "imgui.h"


class ColorPicker : public Widget
{
    public:
        ColorPicker();
        ~ColorPicker();

        void run() override;

    private:
        float _currentColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float _size = 0.5f;
};

#endif /* !ColorPicker_HPP_ */
