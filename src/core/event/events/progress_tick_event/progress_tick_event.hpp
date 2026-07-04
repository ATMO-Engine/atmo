#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"

namespace atmo::core::event::events
{

    class ProgressTickEvent : public event::EventRegistry::Registrable<ProgressTickEvent, Event>
    {
    public:
        using event::EventRegistry::Registrable<ProgressTickEvent, Event>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "ProgressTickEvent";
        }

        float delta_time = 0.0f;
    };

} // namespace atmo::core::event::events
