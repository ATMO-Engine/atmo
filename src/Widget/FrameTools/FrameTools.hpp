#ifndef FrameTools_HPP_
#define FrameTools_HPP_

#include "../Widget.hpp"


class FrameTools : public Widget
{
    public:
        FrameTools();
        ~FrameTools() = default;

        void run() override;
};

#endif /* !FrameTools_HPP_ */
