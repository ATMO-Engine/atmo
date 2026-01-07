#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include "exemple_listener.hpp"


TEST_CASE("Simple Dispatch", "[event]")
{
    ExempleListener listener;

    atmo::core::event::EventDispatcher::Dispatch<EventExemple>(new EventExemple(42));
    REQUIRE(listener.called == true);
}

TEST_CASE("Dispatch Unknown Event", "[event]")
{
    ExempleListener listener;

    atmo::core::event::EventDispatcher::Dispatch(new OtherEvent(200));
    REQUIRE(listener.called == false);
}

TEST_CASE("Unsubscribe Listener", "[event]")
{
    ExempleListener listener;

    atmo::core::event::EventDispatcher::Unsubscribe<EventExemple>(listener);
    atmo::core::event::EventDispatcher::Dispatch<EventExemple>(new EventExemple(100));
    REQUIRE(listener.called == false);
}

TEST_CASE("Different ID", "[event]")
{
    REQUIRE(atmo::core::event::event_id<EventExemple>() != atmo::core::event::event_id<OtherEvent>());
}

TEST_CASE("Consume Event", "[event]")
{
    ExempleListener listener;
    ExempleListener listener2;

    atmo::core::event::EventDispatcher::Dispatch<EventExemple>(new EventExemple(123));
    REQUIRE(listener.called == true);
    REQUIRE(listener2.called == false);
}
