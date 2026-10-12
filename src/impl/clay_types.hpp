#pragma once

#include <vector>
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"

struct ClaySdL3RendererData {
    SDL_Renderer *renderer;
    TTF_TextEngine *text_engine;
};

/**
 * @brief Pass a pointer to an implementation as `.custom.customData` of a Clay element to draw it yourself.
 *
 * `rect` is the element's bounding box in pixels; `dpi_scale` converts Clay (logical) units to pixels.
 */
struct ClayCustomRenderable {
    virtual ~ClayCustomRenderable() = default;
    virtual void render(ClaySdL3RendererData *data, const SDL_FRect &rect, float dpi_scale_x, float dpi_scale_y) = 0;
};
