#pragma once

#include <optional>
#include "SDL3/SDL_keycode.h"
#include "core/text/text_editor.hpp"

namespace atmo::core::text
{
    /**
     * @brief Default key bindings for text editing, following each platform's conventions.
     *
     * macOS: Option = word, Cmd = line/document, Cmd+Z / Shift+Cmd+Z. Other platforms: Ctrl = word/document, Home/End,
     * Ctrl+Z / Ctrl+Y. A code editor can consult its own bindings first and fall back to this one.
     */
    class Keymap
    {
    public:
        enum class Platform {
            MacOS,
            Other,
#if defined(__APPLE__)
            Current = MacOS
#else
            Current = Other
#endif
        };

        static std::optional<EditAction> Lookup(SDL_Keycode key, SDL_Keymod mod, Platform platform = Platform::Current) noexcept;

    private:
        Keymap() = default;
    };
} // namespace atmo::core::text
