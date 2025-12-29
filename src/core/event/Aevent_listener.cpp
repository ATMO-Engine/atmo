#include "Aevent_listener.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            void AListener::callback(event::AEvent *event)
            {
                EventId id = event->id;
                if (handlers.find(id) != handlers.end()) {
                    handlers[id](event);
                }
            }
        } // namespace event
    } // namespace core
} // namespace atmo
