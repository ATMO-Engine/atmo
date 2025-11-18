#include "exemple_listener.hpp"
#include <iostream>


ExempleListener::ExempleListener()
{
    atmo::core::event::EventDispatcher::Subscribe<EventExemple>(*this);
    std::cout << "ExempleListener subscribed to EventExemple events." << std::endl;
}

ExempleListener::~ExempleListener()
{
    atmo::core::event::EventDispatcher::Unsubscribe<EventExemple>(*this);
}

void ExempleListener::onEvent(atmo::core::event::IEvent *event)
{
    const EventExemple *exempleEvent = static_cast<const EventExemple *>(event);
    if (exempleEvent) {
        std::cout << "ExempleListener received EventExemple with value: " << exempleEvent->getExemple() << std::endl;
        called = true;
        event->consume();
    }
}
