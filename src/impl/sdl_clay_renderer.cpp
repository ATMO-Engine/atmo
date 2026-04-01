// This file comes from https://github.com/nicbarker/clay/blob/main/renderers/SDL3/clay_renderer_SDL3.c
// It is adapted to fit imports in this project.

#include <SDL3/SDL.h>
#include <clay.h>
#include <vector>

#include "SDL3/SDL_stdinc.h"
#include "clay_types.hpp"

/* Global for convenience. Even in 4K this is enough for smooth curves (low radius or rect size coupled with
 * no AA or low resolution might make it appear as jagged curves) */
static constexpr int NUM_CIRCLE_SEGMENTS = 16;

// all rendering is performed by a single SDL call, avoiding multiple RenderRect + plumbing choice for circles.
// Outline is built as four corner arcs (clockwise in screen space), then filled via a triangle fan from the center.
static void
SDL_Clay_RenderFillRoundedRect(ClaySdL3RendererData *rendererData, const SDL_FRect rect, const Clay_CornerRadius cornerRadii, const Clay_Color _color)
{
    const SDL_FColor color = { _color.r / 255.0f, _color.g / 255.0f, _color.b / 255.0f, _color.a / 255.0f };

    const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;
    const float tl = SDL_min(cornerRadii.topLeft, minRadius);
    const float tr = SDL_min(cornerRadii.topRight, minRadius);
    const float br = SDL_min(cornerRadii.bottomRight, minRadius);
    const float bl = SDL_min(cornerRadii.bottomLeft, minRadius);

    const float maxRadius = SDL_max(SDL_max(tl, tr), SDL_max(br, bl));
    const int numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (int)(maxRadius * 0.5f));

    // Each of the 4 corners produces (numCircleSegments + 1) arc vertices.
    const int outlineCount = 4 * (numCircleSegments + 1);
    const int totalVertices = 1 + outlineCount; // [0] = center, [1..] = outline
    const int totalIndices = 3 * outlineCount;  // one triangle per outline edge, closing back to start

    std::vector<SDL_Vertex> vertices(totalVertices);
    std::vector<int> indices(totalIndices);

    vertices[0] = SDL_Vertex{ { rect.x + rect.w / 2.0f, rect.y + rect.h / 2.0f }, color, { 0.5f, 0.5f } };

    // Corners are ordered clockwise in screen space (y-down):
    //   TL 180°→270°, TR 270°→360°, BR 0°→90°, BL 90°→180°
    struct Corner {
        float cx, cy, r, start_angle;
    };
    const Corner corners[4] = {
        { rect.x + tl, rect.y + tl, tl, SDL_PI_F },
        { rect.x + rect.w - tr, rect.y + tr, tr, SDL_PI_F * 1.5f },
        { rect.x + rect.w - br, rect.y + rect.h - br, br, 0.0f },
        { rect.x + bl, rect.y + rect.h - bl, bl, SDL_PI_F * 0.5f },
    };

    const float step = (SDL_PI_F / 2.0f) / numCircleSegments;
    int vi = 1;
    for (const Corner &c : corners) {
        for (int i = 0; i <= numCircleSegments; i++) {
            const float angle = c.start_angle + i * step;
            vertices[vi++] = SDL_Vertex{ { c.cx + SDL_cosf(angle) * c.r, c.cy + SDL_sinf(angle) * c.r }, color, { 0.0f, 0.0f } };
        }
    }

    int ii = 0;
    for (int i = 0; i < outlineCount; i++) {
        indices[ii++] = 0;
        indices[ii++] = 1 + i;
        indices[ii++] = 1 + (i + 1) % outlineCount;
    }

    SDL_RenderGeometry(rendererData->renderer, nullptr, vertices.data(), totalVertices, indices.data(), totalIndices);
}

static void SDL_Clay_RenderArc(
    ClaySdL3RendererData *rendererData, const SDL_FPoint center, const float radius, const float startAngle, const float endAngle, const float thickness,
    const Clay_Color color)
{
    SDL_SetRenderDrawColor(rendererData->renderer, (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, (Uint8)color.a);

    const float radStart = startAngle * (SDL_PI_F / 180.0f);
    const float radEnd = endAngle * (SDL_PI_F / 180.0f);

    const int numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (int)(radius * 1.5f)); // increase circle segments for larger circles, 1.5 is arbitrary.

    const float angleStep = (radEnd - radStart) / (float)numCircleSegments;
    const float thicknessStep = 0.4f; // arbitrary value to avoid overlapping lines. Changing THICKNESS_STEP or
                                      // numCircleSegments might cause artifacts.

    for (float t = thicknessStep; t < thickness - thicknessStep; t += thicknessStep) {
        std::vector<SDL_FPoint> points(numCircleSegments + 1);
        const float clampedRadius = SDL_max(radius - t, 1.0f);

        for (int i = 0; i <= numCircleSegments; i++) {
            const float angle = radStart + i * angleStep;
            points[i] = SDL_FPoint{ SDL_roundf(center.x + SDL_cosf(angle) * clampedRadius), SDL_roundf(center.y + SDL_sinf(angle) * clampedRadius) };
        }
        SDL_RenderLines(rendererData->renderer, points.data(), numCircleSegments + 1);
    }
}

static SDL_Rect currentClippingRectangle;

void SDL_Clay_RenderClayCommands(ClaySdL3RendererData *rendererData, Clay_RenderCommandArray *rcommands)
{
    for (size_t i = 0; i < rcommands->length; i++) {
        Clay_RenderCommand *rcmd = Clay_RenderCommandArray_Get(rcommands, i);
        const Clay_BoundingBox bounding_box = rcmd->boundingBox;
        const SDL_FRect rect = { static_cast<float>(bounding_box.x),
                                 static_cast<float>(bounding_box.y),
                                 static_cast<float>(bounding_box.width),
                                 static_cast<float>(bounding_box.height) };

        switch (rcmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
                {
                    Clay_RectangleRenderData *config = &rcmd->renderData.rectangle;
                    SDL_SetRenderDrawBlendMode(rendererData->renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(
                        rendererData->renderer,
                        (Uint8)config->backgroundColor.r,
                        (Uint8)config->backgroundColor.g,
                        (Uint8)config->backgroundColor.b,
                        (Uint8)config->backgroundColor.a);
                    const Clay_CornerRadius &cr = config->cornerRadius;
                    if (cr.topLeft > 0 || cr.topRight > 0 || cr.bottomLeft > 0 || cr.bottomRight > 0) {
                        SDL_Clay_RenderFillRoundedRect(rendererData, rect, cr, config->backgroundColor);
                    } else {
                        SDL_RenderFillRect(rendererData->renderer, &rect);
                    }
                }
                break;
            case CLAY_RENDER_COMMAND_TYPE_TEXT:
                {
                    Clay_TextRenderData *config = &rcmd->renderData.text;
                    TTF_Font *font = rendererData->fonts[config->fontId];
                    TTF_SetFontSize(font, config->fontSize);
                    TTF_Text *text = TTF_CreateText(rendererData->text_engine, font, config->stringContents.chars, config->stringContents.length);
                    TTF_SetTextColor(text, config->textColor.r, config->textColor.g, config->textColor.b, config->textColor.a);
                    TTF_DrawRendererText(text, rect.x, rect.y);
                    TTF_DestroyText(text);
                }
                break;
            case CLAY_RENDER_COMMAND_TYPE_BORDER:
                {
                    Clay_BorderRenderData *config = &rcmd->renderData.border;

                    const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;
                    const Clay_CornerRadius clampedRadii = { .topLeft = SDL_min(config->cornerRadius.topLeft, minRadius),
                                                             .topRight = SDL_min(config->cornerRadius.topRight, minRadius),
                                                             .bottomLeft = SDL_min(config->cornerRadius.bottomLeft, minRadius),
                                                             .bottomRight = SDL_min(config->cornerRadius.bottomRight, minRadius) };
                    // edges
                    SDL_SetRenderDrawColor(
                        rendererData->renderer, (Uint8)config->color.r, (Uint8)config->color.g, (Uint8)config->color.b, (Uint8)config->color.a);
                    if (config->width.left > 0) {
                        const float starting_y = rect.y + clampedRadii.topLeft;
                        const float length = rect.h - clampedRadii.topLeft - clampedRadii.bottomLeft;
                        SDL_FRect line = { rect.x - 1, starting_y, static_cast<float>(config->width.left), length };
                        SDL_RenderFillRect(rendererData->renderer, &line);
                    }
                    if (config->width.right > 0) {
                        const float starting_x = rect.x + rect.w - (float)config->width.right + 1;
                        const float starting_y = rect.y + clampedRadii.topRight;
                        const float length = rect.h - clampedRadii.topRight - clampedRadii.bottomRight;
                        SDL_FRect line = { starting_x, starting_y, static_cast<float>(config->width.right), length };
                        SDL_RenderFillRect(rendererData->renderer, &line);
                    }
                    if (config->width.top > 0) {
                        const float starting_x = rect.x + clampedRadii.topLeft;
                        const float length = rect.w - clampedRadii.topLeft - clampedRadii.topRight;
                        SDL_FRect line = { starting_x, rect.y - 1, length, static_cast<float>(config->width.top) };
                        SDL_RenderFillRect(rendererData->renderer, &line);
                    }
                    if (config->width.bottom > 0) {
                        const float starting_x = rect.x + clampedRadii.bottomLeft;
                        const float starting_y = rect.y + rect.h - (float)config->width.bottom + 1;
                        const float length = rect.w - clampedRadii.bottomLeft - clampedRadii.bottomRight;
                        SDL_FRect line = { starting_x, starting_y, length, static_cast<float>(config->width.bottom) };
                        SDL_RenderFillRect(rendererData->renderer, &line);
                    }
                    // corners
                    if (config->cornerRadius.topLeft > 0) {
                        const float centerX = rect.x + clampedRadii.topLeft - 1;
                        const float centerY = rect.y + clampedRadii.topLeft - 1;
                        SDL_Clay_RenderArc(
                            rendererData, SDL_FPoint{ centerX, centerY }, clampedRadii.topLeft, 180.0f, 270.0f, config->width.top, config->color);
                    }
                    if (config->cornerRadius.topRight > 0) {
                        const float centerX = rect.x + rect.w - clampedRadii.topRight;
                        const float centerY = rect.y + clampedRadii.topRight - 1;
                        SDL_Clay_RenderArc(
                            rendererData, SDL_FPoint{ centerX, centerY }, clampedRadii.topRight, 270.0f, 360.0f, config->width.top, config->color);
                    }
                    if (config->cornerRadius.bottomLeft > 0) {
                        const float centerX = rect.x + clampedRadii.bottomLeft - 1;
                        const float centerY = rect.y + rect.h - clampedRadii.bottomLeft;
                        SDL_Clay_RenderArc(
                            rendererData, SDL_FPoint{ centerX, centerY }, clampedRadii.bottomLeft, 90.0f, 180.0f, config->width.bottom, config->color);
                    }
                    if (config->cornerRadius.bottomRight > 0) {
                        const float centerX = rect.x + rect.w - clampedRadii.bottomRight;
                        const float centerY = rect.y + rect.h - clampedRadii.bottomRight;
                        SDL_Clay_RenderArc(
                            rendererData, SDL_FPoint{ centerX, centerY }, clampedRadii.bottomRight, 0.0f, 90.0f, config->width.bottom, config->color);
                    }
                }
                break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
                {
                    Clay_BoundingBox boundingBox = rcmd->boundingBox;
                    currentClippingRectangle = SDL_Rect{
                        .x = static_cast<int>(boundingBox.x),
                        .y = static_cast<int>(boundingBox.y),
                        .w = static_cast<int>(boundingBox.width),
                        .h = static_cast<int>(boundingBox.height),
                    };
                    SDL_SetRenderClipRect(rendererData->renderer, &currentClippingRectangle);
                    break;
                }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
                {
                    SDL_SetRenderClipRect(rendererData->renderer, nullptr);
                    break;
                }
            case CLAY_RENDER_COMMAND_TYPE_IMAGE:
                {
                    SDL_Texture *texture = (SDL_Texture *)rcmd->renderData.image.imageData;
                    SDL_RenderTexture(rendererData->renderer, texture, nullptr, &rect);
                    break;
                }
            default:
                SDL_Log("Unknown render command type: %d", rcmd->commandType);
        }
    }
}
