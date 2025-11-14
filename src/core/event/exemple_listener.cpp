#include "exemple_listener.hpp"
#include <iostream>
#include "core/event/event_dispatcher.hpp"

namespace atmo
{
    namespace core
    {
        namespace event
        {
            ExempleListener::ExempleListener()
            {
                EventDispatcher::getInstance().subscribe<EventExemple>(*this);
                std::cout << "ExempleListener subscribed to EventExemple events." << std::endl;
            }

            ExempleListener::~ExempleListener()
            {
                EventDispatcher::getInstance().unsubscribe<EventExemple>(*this);
            }

            void ExempleListener::onEvent(const IEvent &event)
            {
                const EventExemple &exempleEvent = static_cast<const EventExemple &>(event);
                this->called = true;
                int value = exempleEvent.getExemple();
                // spdlog::info("Received EventExemple with value: {}", value);
                std::cout << "ExempleListener received EventExemple with value: " << value << std::endl;
            }
        } // namespace event
    } // namespace core
} // namespace atmo
