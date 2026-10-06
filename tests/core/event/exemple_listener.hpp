#pragma once

#include <string_view>

#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"

class EventExample : public atmo::core::event::EventRegistry::Registrable<EventExample, atmo::core::event::events::Event>
{
public:
    EventExample(int exemple = 0) : example(exemple) {}

    static constexpr std::string_view LocalName()
    {
        return "EventExample";
    }

    int example;
};

class OtherEvent : public atmo::core::event::EventRegistry::Registrable<OtherEvent, atmo::core::event::events::Event>
{
public:
    OtherEvent(int value = 0) {}

    static constexpr std::string_view LocalName()
    {
        return "OtherEvent";
    }

    int value = 0;
};
