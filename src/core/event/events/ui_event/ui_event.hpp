#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"

namespace atmo::core::event::events
{

    class UIEvent : public event::EventRegistry::Registrable<UIEvent, Event>
    {
    public:
        using event::EventRegistry::Registrable<UIEvent, Event>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "UIEvent";
        }

    private:
        flecs::entity m_entity_id;
    };

} // namespace atmo::core::event::events
