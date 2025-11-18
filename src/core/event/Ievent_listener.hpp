#pragma once

#include <any>
#include "core/event/Ievent.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            class IListener
            {
            public:
                virtual ~IListener() = default;
                virtual void onEvent(IEvent *event) = 0;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
