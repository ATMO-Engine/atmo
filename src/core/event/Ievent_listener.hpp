#pragma once

#include <functional>
#include <unordered_map>
#include "core/event/Ievent.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            using EventId = uint32_t;
            class IListener
            {
            public:
                virtual ~IListener() = default;
                virtual void onEvent(IEvent *event) = 0;
                std::unordered_map<EventId, std::function<void(event::IEvent *)>> handlers;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
