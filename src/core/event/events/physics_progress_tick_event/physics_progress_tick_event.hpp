#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"

namespace atmo::core::event::events
{

    class PhysicsProgressTickEvent : public event::EventRegistry::Registrable<PhysicsProgressTickEvent, Event>
    {
    public:
        using event::EventRegistry::Registrable<PhysicsProgressTickEvent, Event>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "PhysicsProgressTickEvent";
        }

        float delta_time = 0.0f;
    };

} // namespace atmo::core::event::events
