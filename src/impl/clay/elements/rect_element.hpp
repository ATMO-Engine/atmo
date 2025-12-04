#pragma once

#include <clay.h>
#include <string>
#include "core/ecs/components.hpp"
#include "impl/clay/Aclay_element.hpp"

namespace atmo
{
    namespace impl
    {
        namespace clay
        {
            class RectElement : public Aclay_element
            {
            public:
                RectElement(std::string id, Clay_Sizing size, Clay_Color color) : m_id(id), p_sizing(size), p_color(color) {};
                ~RectElement() override = default;

                void componentContent() override;

            protected:
                Clay_Sizing p_sizing;
                Clay_Color p_color;

            private:
                std::string m_id;
            };
        } // namespace clay
    } // namespace impl
} // namespace atmo
