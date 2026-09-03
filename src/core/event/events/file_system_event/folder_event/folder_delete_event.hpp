#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/file_system_event.hpp"

namespace atmo::core::event::events
{

    class FolderDeleteEvent : public event::EventRegistry::Registrable<FolderDeleteEvent, FileSystemEvent>
    {
    public:
        using event::EventRegistry::Registrable<FolderDeleteEvent, FileSystemEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "FolderDeleteEvent";
        }

    };

} // namespace atmo::core::event::events
