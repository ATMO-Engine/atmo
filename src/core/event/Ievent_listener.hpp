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
                std::unordered_map<EventId, std::function<void(event::IEvent *)>> handlers;
                void callback(event::IEvent *event);
            };
        } // namespace event
    } // namespace core
} // namespace atmo
