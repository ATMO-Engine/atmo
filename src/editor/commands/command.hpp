#pragma once

#include <functional>
#include <optional>
#include <string>

#include "SDL3/SDL_keycode.h"

#if !defined(ATMO_EXPORT)

namespace atmo::editor
{
    struct Shortcut {
        SDL_Keycode key;
        SDL_Keymod modifiers;
    };

    struct Command {
        std::string id;                       // "file.save" — also used as TR() key
        std::string category;                 // "file" — TR() key for the top-level menu label
        std::optional<Shortcut> shortcut;     // for display in menus only
        std::optional<std::string> icon_path; // "project://icons/save.png"
        std::function<void()> action;
    };
} // namespace atmo::editor

#endif
