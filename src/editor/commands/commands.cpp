#include "commands.hpp"

#if !defined(ATMO_EXPORT)

#include <algorithm>
#include <cctype>

#include "spdlog/spdlog.h"

namespace atmo::editor
{
    namespace
    {
        SDL_Keymod NormalizeModifiers(SDL_Keymod modifiers)
        {
            Uint16 normalized = 0;
            if (modifiers & SDL_KMOD_CTRL)
                normalized |= SDL_KMOD_CTRL;
            if (modifiers & SDL_KMOD_SHIFT)
                normalized |= SDL_KMOD_SHIFT;
            if (modifiers & SDL_KMOD_ALT)
                normalized |= SDL_KMOD_ALT;
            if (modifiers & SDL_KMOD_GUI)
                normalized |= SDL_KMOD_GUI;
            return static_cast<SDL_Keymod>(normalized);
        }
    } // namespace

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

    const Command *Commands::findByShortcut(SDL_Keycode key, SDL_Keymod modifiers) const
    {
        SDL_Keymod normalized = NormalizeModifiers(modifiers);
        for (const Command &cmd : m_commands) {
            if (cmd.shortcut && cmd.shortcut->key == key && NormalizeModifiers(cmd.shortcut->modifiers) == normalized)
                return &cmd;
        }
        return nullptr;
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
