#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/file_system_event.hpp"

namespace atmo::core::event::events
{

    class FolderMoveEvent : public event::EventRegistry::Registrable<FolderMoveEvent, FileSystemEvent>
    {
    public:
        using event::EventRegistry::Registrable<FolderMoveEvent, FileSystemEvent>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "FolderMoveEvent";
        }

    };

} // namespace atmo::core::event::events
