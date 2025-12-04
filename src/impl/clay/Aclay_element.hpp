#pragma once


namespace atmo
{
    namespace impl
    {
        namespace clay
        {
            class Aclay_element
            {
            public:
                virtual ~Aclay_element() = default;
                virtual void componentContent() = 0;
            };
        } // namespace clay
    } // namespace impl
} // namespace atmo
