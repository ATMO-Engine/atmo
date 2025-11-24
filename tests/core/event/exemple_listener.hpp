#pragma once

#include "core/event/Aevent_listener.hpp"
#include "core/event/event_dispatcher.hpp"


class EventExemple : public atmo::core::event::AEvent
{
public:
    EventExemple(int exemple) : m_exemple(exemple)
    {
        id = atmo::core::event::event_id<EventExemple>();
    }
    ~EventExemple() override = default;

    int getExemple() const
    {
        return m_exemple;
    }

private:
    int m_exemple;
};

class OtherEvent : public atmo::core::event::AEvent
{
public:
    OtherEvent(int value) : m_value(value) {}
    int getValue() const
    {
        return m_value;
    }

private:
    int m_value;
};

class ExempleListener : public atmo::core::event::AListener
{
public:
    bool called = false;
    ExempleListener();
    ~ExempleListener() override;
    void onEvent(atmo::core::event::AEvent *event);
};
