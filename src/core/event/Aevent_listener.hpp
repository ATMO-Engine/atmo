#pragma once

#include <functional>
#include <unordered_map>
#include "core/event/Aevent.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            using EventId = uint32_t;
            class AListener
            {
            public:
                virtual ~AListener() = default;
                std::unordered_map<EventId, std::function<void(event::AEvent *)>> handlers;
                void callback(event::AEvent *event);
            };
        } // namespace event
    } // namespace core
} // namespace atmo
