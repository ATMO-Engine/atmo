#pragma once

#include "SDL3/SDL.h"

struct OffscreenVideo {
    OffscreenVideo()
    {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
        available = SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    ~OffscreenVideo()
    {
        if (available)
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    OffscreenVideo(const OffscreenVideo &) = delete;
    OffscreenVideo &operator=(const OffscreenVideo &) = delete;

    bool available = false;
};
