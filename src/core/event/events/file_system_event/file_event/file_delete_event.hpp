#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/file_system_event.hpp"

namespace atmo::core::event::events
{

    class FileDeleteEvent : public event::EventRegistry::Registrable<FileDeleteEvent, FileSystemEvent>
    {
    public:
        using event::EventRegistry::Registrable<FileDeleteEvent, FileSystemEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "FileDeleteEvent";
        }

    };

} // namespace atmo::core::event::events
