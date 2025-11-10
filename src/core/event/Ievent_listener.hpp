#pragma once

#include <any>
#include "core/event/event_dispatcher.hpp"
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
                virtual void onEvent(const IEvent &event) = 0;
            };

            class ExempleListener : public IListener
            {
            public:
                ExempleListener(EventDispatcher &dispatcher)
                {
                    dispatcher.subscribe<EventExemple>(*this);
                };
                virtual ~ExempleListener() = default;
                virtual void onEvent(const IEvent &event) override = 0;
                EventExemple receiveEvent(int exemple) {}
            };
        } // namespace event
    } // namespace core
} // namespace atmo
