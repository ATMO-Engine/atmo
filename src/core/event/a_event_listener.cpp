#include "a_event_listener.hpp"
#include "core/event/a_event.hpp"
#include "core/event/event_dispatcher.hpp"


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

            ClayListener::ClayListener()
            {
                atmo::core::event::EventDispatcher::Subscribe<UIEvent>(*this);
                atmo::core::event::EventDispatcher::Subscribe<HoverEvent>(*this);
                // handlers[atmo::core::event::event_id<UIEvent>()] = [this](event::UIEvent *event) { onEvent(event); };
                // handlers[atmo::core::event::event_id<HoverEvent>()] = [this](event::HoverEvent *event) { hoverEvent(event); };
            }
        } // namespace event
    } // namespace core
} // namespace atmo
