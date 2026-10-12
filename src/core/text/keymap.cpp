#include "keymap.hpp"

namespace atmo::core::text
{
    std::optional<EditAction> Keymap::Lookup(SDL_Keycode key, SDL_Keymod mod, Platform platform) noexcept
    {
        const bool shift = (mod & SDL_KMOD_SHIFT) != 0;
        const bool ctrl = (mod & SDL_KMOD_CTRL) != 0;
        const bool alt = (mod & SDL_KMOD_ALT) != 0;
        const bool gui = (mod & SDL_KMOD_GUI) != 0;
        const bool mac = platform == Platform::MacOS;

        const bool primary = mac ? gui : ctrl;
        const bool word = mac ? alt : ctrl;

        using C = EditCommand;
        auto act = [shift](C command) { return EditAction{ command, shift }; };

        switch (key) {
            case SDLK_LEFT:
                if (mac && gui)
                    return act(C::MoveLineStart);
                return act(word ? C::MoveWordLeft : C::MoveLeft);
            case SDLK_RIGHT:
                if (mac && gui)
                    return act(C::MoveLineEnd);
                return act(word ? C::MoveWordRight : C::MoveRight);
            case SDLK_UP:
                if (mac && gui)
                    return act(C::MoveDocStart);
                return act(C::MoveUp);
            case SDLK_DOWN:
                if (mac && gui)
                    return act(C::MoveDocEnd);
                return act(C::MoveDown);
            case SDLK_HOME:
                return act(!mac && ctrl ? C::MoveDocStart : C::MoveLineStart);
            case SDLK_END:
                return act(!mac && ctrl ? C::MoveDocEnd : C::MoveLineEnd);
            case SDLK_PAGEUP:
                return act(C::MovePageUp);
            case SDLK_PAGEDOWN:
                return act(C::MovePageDown);

            case SDLK_BACKSPACE:
                if (mac && gui)
                    return EditAction{ C::DeleteToLineStart };
                return EditAction{ word ? C::DeleteWordBack : C::Backspace };
            case SDLK_DELETE:
                if (!mac && shift && !ctrl)
                    return EditAction{ C::Cut };
                if (mac && gui)
                    return EditAction{ C::DeleteToLineEnd };
                return EditAction{ word ? C::DeleteWordForward : C::Delete };
            case SDLK_INSERT:
                if (mac)
                    return std::nullopt;
                if (ctrl)
                    return EditAction{ C::Copy };
                if (shift)
                    return EditAction{ C::Paste };
                return std::nullopt;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                return EditAction{ C::InsertNewline };
            case SDLK_TAB:
                if (ctrl || alt || gui)
                    return std::nullopt;
                return EditAction{ C::InsertTab };

            default:
                break;
        }

        if (mac && ctrl && !gui && !alt) {
            if (key == SDLK_A)
                return act(C::MoveLineStart);
            if (key == SDLK_E)
                return act(C::MoveLineEnd);
            if (key == SDLK_K)
                return EditAction{ C::DeleteToLineEnd };
        }

        if (!primary || alt)
            return std::nullopt;

        switch (key) {
            case SDLK_A:
                return EditAction{ C::SelectAll };
            case SDLK_C:
                return EditAction{ C::Copy };
            case SDLK_X:
                return EditAction{ C::Cut };
            case SDLK_V:
                return EditAction{ C::Paste };
            case SDLK_Z:
                return EditAction{ shift ? C::Redo : C::Undo };
            case SDLK_Y:
                if (!mac)
                    return EditAction{ C::Redo };
                return std::nullopt;
            default:
                return std::nullopt;
        }
    }
} // namespace atmo::core::text
