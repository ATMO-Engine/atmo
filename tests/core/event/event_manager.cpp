#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include "exemple_listener.hpp"

// std::cout << "Unsubscribing and dispatching EventExemple again" << std::endl;
// atmo::core::event::EventDispatcher::getInstance().unsubscribe<atmo::core::event::EventExemple>(listener);
// atmo::core::event::EventDispatcher::getInstance().dispatch<atmo::core::event::EventExemple>(event);
// std::cout << "Listener called: " << (listener.called ? "true" : "false") << std::endl;

// std::cout << "EventExemple ID  : " << atmo::core::event::event_id<atmo::core::event::EventExemple>() << "\n";
// std::cout << "OtherEvent ID    : " << atmo::core::event::event_id<OtherEvent>() << "\n";

// if (atmo::core::event::event_id<atmo::core::event::EventExemple>() == atmo::core::event::event_id<OtherEvent>()) {
//     std::cout << "IDs identiques";
// } else {
//     std::cout << "IDs différents.\n";
// }


TEST_CASE("Simple Dispatch", "[event]")
{

    EventExemple event(42);
    ExempleListener listener;

    atmo::core::event::EventDispatcher::getInstance().dispatch<EventExemple>(event);
    REQUIRE(listener.called == true);
}

TEST_CASE("Dispatch Unknown Event", "[event]")
{
    EventExemple event(100);
    ExempleListener listener;

    OtherEvent otherEvent(200);
    atmo::core::event::EventDispatcher::getInstance().dispatch(otherEvent);

    REQUIRE(listener.called == false);
}
