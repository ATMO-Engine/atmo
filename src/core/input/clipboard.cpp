#include "clipboard.hpp"
#include "SDL3/SDL_clipboard.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"
#include "spdlog/spdlog.h"

namespace atmo::core
{
    void SdlClipboard::setText(std::string_view text)
    {
        std::string copy(text);
        if (!SDL_SetClipboardText(copy.c_str()))
            spdlog::error("Failed to set clipboard text: {}", SDL_GetError());
    }

    std::string SdlClipboard::getText() const
    {
        char *raw = SDL_GetClipboardText();
        if (!raw)
            return {};
        std::string out(raw);
        SDL_free(raw);
        return out;
    }
} // namespace atmo::core
