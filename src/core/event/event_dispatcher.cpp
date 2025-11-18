#include "event_dispatcher.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            EventDispatcher &EventDispatcher::getInstance()
            {
                static EventDispatcher instance;
                return instance;
            }
        } // namespace event
    } // namespace core
} // namespace atmo
