#include "event.hpp"
#include "core/event/event_registry.hpp"

namespace atmo::core::event::events
{
    bool Event::isConsumed() const
    {
        return m_consumed;
    }

    void Event::consume()
    {
        m_consumed = true;
    }
} // namespace atmo::core::event::events


REGISTER_EVENT(Event);
