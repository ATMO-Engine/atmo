#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"
#include "file_watcher/file_watcher.hpp"

namespace atmo::core::event::events
{

    class ReloadExplorerEvent : public event::EventRegistry::Registrable<ReloadExplorerEvent, Event>
    {
    public:
        using event::EventRegistry::Registrable<ReloadExplorerEvent, Event>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "ReloadExplorerEvent";
        }
    };

} // namespace atmo::core::event::events
