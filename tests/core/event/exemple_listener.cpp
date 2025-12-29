#include "exemple_listener.hpp"
#include <iostream>


ExempleListener::ExempleListener()
{
    atmo::core::event::EventDispatcher::Subscribe<EventExemple>(*this);
    handlers[atmo::core::event::event_id<EventExemple>()] = [this](atmo::core::event::AEvent *event) { onEvent(event); };
    std::cout << "ExempleListener subscribed to EventExemple events." << std::endl;
}

ExempleListener::~ExempleListener()
{
    atmo::core::event::EventDispatcher::Unsubscribe<EventExemple>(*this);
}

void ExempleListener::onEvent(atmo::core::event::AEvent *event)
{
    const EventExemple *exempleEvent = static_cast<const EventExemple *>(event);
    if (exempleEvent) {
        std::cout << "ExempleListener received EventExemple with value: " << exempleEvent->getExemple() << std::endl;
        called = true;
        event->consume();
    }
}
