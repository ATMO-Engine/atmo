#include "Ievent_listener.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            void IListener::callback(event::IEvent *event)
            {
                EventId id = event->id;
                if (handlers.find(id) != handlers.end()) {
                    handlers[id](event);
                }
            }
        } // namespace event
    } // namespace core
} // namespace atmo
