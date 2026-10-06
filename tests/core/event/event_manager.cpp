#include <catch2/catch_test_macros.hpp>

#include "exemple_listener.hpp"

using atmo::core::event::EventRegistry;

TEST_CASE("Simple Dispatch", "[event]")
{
    auto example = atmo::core::event::EventRegistry::Create<EventExample>("Event::EventExample");
    example->example = 1;
    REQUIRE(example != nullptr);

    auto id = atmo::core::event::EventRegistry::SetCallBack<EventExample>([](EventExample *evt) {
        REQUIRE(evt->example == 1);
    });

    EventRegistry::RemoveCallBack<EventExample>(id);
}

TEST_CASE("Doesn't trigger other event", "[event]")
{
    bool example_called = false;
    bool other_called = false;

    auto id1 = EventRegistry::SetCallBack<EventExample>([&example_called](EventExample *) { example_called = true; });
    auto id2 = EventRegistry::SetCallBack<OtherEvent>([&other_called](OtherEvent *) { other_called = true; });

    auto example = EventRegistry::Create<EventExample>("Event::EventExample");
    EventRegistry::Dispatch(example);

    EventRegistry::RemoveCallBack<EventExample>(id1);
    EventRegistry::RemoveCallBack<EventExample>(id2);

    REQUIRE(example_called);
    REQUIRE_FALSE(other_called);
}

TEST_CASE("Remove callback", "[event]")
{
    bool example_called = false;

    auto example_id = EventRegistry::SetCallBack<EventExample>([&example_called](EventExample *) { example_called = true; });

    auto example = EventRegistry::Create<EventExample>("Event::EventExample");

    EventRegistry::RemoveCallBack<EventExample>(example_id);
    EventRegistry::Dispatch(example);

    REQUIRE_FALSE(example_called);
}

TEST_CASE("Consume callback", "[event]")
{
    bool first_called = false;
    bool second_called = false;

    auto id1 = EventRegistry::SetCallBack<EventExample>([&first_called](EventExample *evt) {
        first_called = true;
        evt->consume();
    });
    auto id2 = EventRegistry::SetCallBack<EventExample>([&second_called](EventExample *) { second_called = true; });

    auto example = EventRegistry::Create<EventExample>("Event::EventExample");
    EventRegistry::Dispatch(example);

    EventRegistry::RemoveCallBack<EventExample>(id1);
    EventRegistry::RemoveCallBack<EventExample>(id2);

    REQUIRE(first_called);
    REQUIRE_FALSE(second_called);
    REQUIRE(example->isConsumed());
}
