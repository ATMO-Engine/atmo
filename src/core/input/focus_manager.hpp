#pragma once

#include <cstdint>
#include <functional>
#include "SDL3/SDL_video.h"
#include "core/text/text_editor.hpp"

namespace atmo::core
{
    /**
     * @brief Tracks which entity owns keyboard focus. Only one text input can be focused at a time.
     *
     * Focusing starts SDL text input on the target's window; blurring stops it and notifies the previous owner.
     */
    class FocusManager
    {
    public:
        struct Target {
            std::uint64_t id = 0;
            SDL_Window *window = nullptr;
            /// Called when focus is taken away (click elsewhere, another input focused, explicit blur).
            std::function<void()> on_blur;
            /// Executes an editing action on the target, used to route menu commands (undo, copy...) to it.
            std::function<bool(text::EditAction)> on_action;
        };

        static void Focus(Target target);
        /// Removes focus from the current owner, calling its `on_blur`.
        static void Blur();
        /// Drops focus without calling `on_blur`, used when the owner is being destroyed.
        static void Release(std::uint64_t id) noexcept;

        static bool IsFocused(std::uint64_t id) noexcept;
        static bool HasTextFocus() noexcept;
        static std::uint64_t GetFocusedId() noexcept;

        /// Forwards an editing action to the focused input. Returns false when nothing is focused or it was not handled.
        static bool DispatchEditAction(text::EditAction action);

    private:
        FocusManager() = default;
        static FocusManager &Instance();

        Target m_target;
    };
} // namespace atmo::core
