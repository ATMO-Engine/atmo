#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/file_system_event.hpp"

namespace atmo::core::event::events
{

    class FileCreateEvent : public event::EventRegistry::Registrable<FileCreateEvent, FileSystemEvent>
    {
    public:
        using event::EventRegistry::Registrable<FileCreateEvent, FileSystemEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "FileCreateEvent";
        }

    };

} // namespace atmo::core::event::events
