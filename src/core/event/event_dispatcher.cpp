#include "event_dispatcher.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            EventDispatcher &EventDispatcher::GetInstance()
            {
                static EventDispatcher instance;
                return instance;
            }
        } // namespace event
    } // namespace core
} // namespace atmo
