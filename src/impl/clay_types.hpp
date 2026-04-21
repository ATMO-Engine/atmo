#pragma once

#include <vector>
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"

struct ClaySdL3RendererData {
    SDL_Renderer *renderer;
    TTF_TextEngine *text_engine;
};
