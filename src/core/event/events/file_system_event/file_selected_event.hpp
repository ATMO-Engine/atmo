#pragma once

#include <filesystem>
#include <string_view>
#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"

namespace atmo::core::event::events
{

    class FileSelectedEvent : public event::EventRegistry::Registrable<FileSelectedEvent, Event>
    {
    public:
        using event::EventRegistry::Registrable<FileSelectedEvent, Event>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "FileSelectedEvent";
        }

        std::filesystem::path path;
    };

} // namespace atmo::core::event::events
