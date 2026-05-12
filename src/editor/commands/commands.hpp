#pragma once

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "editor/commands/command.hpp"

#if !defined(ATMO_EXPORT)

namespace atmo::editor
{
    class Commands
    {
    public:
        void registerCommand(Command command);

        bool execute(const std::string &id) const;

        const std::vector<Command> &all() const noexcept;

        std::vector<const Command *> search(std::string_view query) const;

        std::map<std::string, std::vector<const Command *>> byCategory() const;

    private:
        std::vector<Command> m_commands;
        std::unordered_map<std::string, size_t> m_index;
    };
} // namespace atmo::editor

#endif
