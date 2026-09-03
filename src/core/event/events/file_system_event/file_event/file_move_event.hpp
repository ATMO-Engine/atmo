#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/file_system_event.hpp"

namespace atmo::core::event::events
{

    class FileMoveEvent : public event::EventRegistry::Registrable<FileMoveEvent, FileSystemEvent>
    {
    public:
        using event::EventRegistry::Registrable<FileMoveEvent, FileSystemEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "FileMoveEvent";
        }

    };

} // namespace atmo::core::event::events
