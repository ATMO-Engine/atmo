#pragma once

#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"
#include "file_watcher/file_watcher.hpp"

namespace atmo::core::event::events
{

    class FileSystemEvent : public event::EventRegistry::Registrable<FileSystemEvent, Event>
    {
    public:
        using event::EventRegistry::Registrable<FileSystemEvent, Event>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "FileSystemEvent";
        }

        atmo::FileWatcher::FileChange change;
    };

} // namespace atmo::core::event::events
