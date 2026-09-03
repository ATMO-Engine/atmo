#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/file_system_event.hpp"

namespace atmo::core::event::events
{

    class FolderCreateEvent : public event::EventRegistry::Registrable<FolderCreateEvent, FileSystemEvent>
    {
    public:
        using event::EventRegistry::Registrable<FolderCreateEvent, FileSystemEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "FolderCreateEvent";
        }

    };

} // namespace atmo::core::event::events
