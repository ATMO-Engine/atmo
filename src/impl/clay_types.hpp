#pragma once

#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"

typedef struct
{
    SDL_Renderer *renderer;
    TTF_TextEngine *textEngine;
    TTF_Font **fonts;
} Clay_SDL3RendererData;
