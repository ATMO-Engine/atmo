#include "exemple_listener.hpp"
#include <iostream>


ExempleListener::ExempleListener()
{
    atmo::core::event::EventDispatcher::getInstance().subscribe<EventExemple>(*this);
    std::cout << "ExempleListener subscribed to EventExemple events." << std::endl;
}

ExempleListener::~ExempleListener()
{
    atmo::core::event::EventDispatcher::getInstance().unsubscribe<EventExemple>(*this);
}

void ExempleListener::onEvent(const atmo::core::event::IEvent &event)
{
    const EventExemple &exempleEvent = static_cast<const EventExemple &>(event);
    this->called = true;
    int value = exempleEvent.getExemple();
    // spdlog::info("Received EventExemple with value: {}", value);
    std::cout << "ExempleListener received EventExemple with value: " << value << std::endl;
}
