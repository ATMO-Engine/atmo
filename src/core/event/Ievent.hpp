#pragma once

#include <any>

namespace atmo
{
    namespace core
    {
        namespace event
        {
            class IEvent
            {
            public:
                virtual ~IEvent() = default;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
