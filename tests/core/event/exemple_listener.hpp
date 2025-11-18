#pragma once

#include "core/event/Ievent_listener.hpp"
#include "core/event/event_dispatcher.hpp"


class EventExemple : public atmo::core::event::IEvent
{
public:
    EventExemple(int exemple) : m_exemple(exemple) {}
    ~EventExemple() override = default;

    int getExemple() const
    {
        return m_exemple;
    }

private:
    int m_exemple;
};

class OtherEvent : public atmo::core::event::IEvent
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

class ExempleListener : public atmo::core::event::IListener
{
public:
    bool called = false;
    ExempleListener();
    ~ExempleListener() override;
    void onEvent(atmo::core::event::IEvent *event) override;
};
