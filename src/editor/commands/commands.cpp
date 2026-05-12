#include "commands.hpp"

#if !defined(ATMO_EXPORT)

#include <algorithm>
#include <cctype>

#include "spdlog/spdlog.h"

namespace atmo::editor
{
    void Commands::registerCommand(Command command)
    {
        if (m_index.contains(command.id)) {
            spdlog::warn("Command '{}' is already registered, skipping", command.id);
            return;
        }
        m_index[command.id] = m_commands.size();
        m_commands.push_back(std::move(command));
    }

    bool Commands::execute(const std::string &id) const
    {
        auto it = m_index.find(id);
        if (it == m_index.end()) {
            spdlog::warn("Command '{}' not found", id);
            return false;
        }
        const Command &cmd = m_commands[it->second];
        if (cmd.action)
            cmd.action();
        return true;
    }

    const std::vector<Command> &Commands::all() const noexcept
    {
        return m_commands;
    }

    std::vector<const Command *> Commands::search(std::string_view query) const
    {
        std::vector<const Command *> results;

        auto lower = [](std::string_view sv) {
            std::string s(sv);
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            return s;
        };

        std::string q = lower(query);

        for (const Command &cmd : m_commands) {
            if (lower(cmd.id).contains(q))
                results.push_back(&cmd);
        }
        return results;
    }

    std::map<std::string, std::vector<const Command *>> Commands::byCategory() const
    {
        std::map<std::string, std::vector<const Command *>> result;
        for (const Command &cmd : m_commands) result[cmd.category].push_back(&cmd);
        return result;
    }
} // namespace atmo::editor

#endif
