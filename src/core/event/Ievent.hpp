#pragma once

#include <any>
#include "core/event/event_dispatcher.hpp"
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

            class EventExemple : public IEvent
            {
            public:
                EventExemple(int exemple) : m_exemple(exemple) {}
                ~EventExemple() override = default;

                int getExemple() const
                {
                    return m_exemple;
                }

            private:
                int m_exemple;
            };
        } // namespace event
    } // namespace core
} // namespace atmo
