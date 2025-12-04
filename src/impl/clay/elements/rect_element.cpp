extern "C" {
#include <clay.h>
}

#include "rect_element.hpp"

namespace atmo
{
    namespace impl
    {
        namespace clay
        {
            void RectElement::componentContent()
            {
                Clay_ElementConfig config{};
                config.layout.sizing = p_sizing;
                config.backgroundColor = p_color;

                CLAY(CLAY_ID(m_id.c_str()), config);
            }
        } // namespace clay
    } // namespace impl
} // namespace atmo
