#include "ui_drawing_canvas.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "meta/auto_register.hpp"

#include "core/input/input_manager.hpp"
#include "common/math.hpp"

namespace atmo::core::ecs::entities
{
    void UIDrawingCanvas::RegisterSystems(flecs::world *world)
    {
    }

    void UIDrawingCanvas::initialize()
    {
        UI::initialize();
        setComponent<components::UIDrawingCanvas>({});
    }

    Clay_ElementDeclaration UIDrawingCanvas::buildDecl()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        return Clay_ElementDeclaration{
            .id = CLAY_ID("DrawingCanvas"),
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(comp.canvasSize.x),
                    .height = CLAY_SIZING_FIXED(comp.canvasSize.y),
                }
            }
        };
    }

    float UIDrawingCanvas::computeFitScale(const components::UIDrawingCanvas &comp) const
    {
        if (comp.textureSize.x <= 0 || comp.textureSize.y <= 0) {
            return 1;
        }

        float scaleX = comp.bounds.width  / comp.textureSize.x;
        float scaleY = comp.bounds.height / comp.textureSize.y;

        return std::min(scaleX, scaleY);
    }

    SDL_FRect UIDrawingCanvas::computeTextureRect(const components::UIDrawingCanvas &comp) const
    {
        float scale = computeFitScale(comp) * comp.zoom;

        float drawW = comp.textureSize.x * scale;
        float drawH = comp.textureSize.y * scale;

        float drawX = comp.bounds.x + (comp.bounds.width - drawW) / 2.0f + comp.offset.x;
        float drawY = comp.bounds.y + (comp.bounds.height - drawH) / 2.0f + comp.offset.y;

        return { drawX, drawY, drawW, drawH };
    }

    bool UIDrawingCanvas::isInsideTextureRect(atmo::core::types::Vector2 screenPos, const components::UIDrawingCanvas &comp) const
    {
        SDL_FRect rect = computeTextureRect(comp);
        return screenPos.x >= rect.x && screenPos.x < rect.x + rect.w &&
               screenPos.y >= rect.y && screenPos.y < rect.y + rect.h;
    }

    void UIDrawingCanvas::clampOffset(components::UIDrawingCanvas &comp)
    {
        SDL_FRect textureRect = computeTextureRect(comp);

        float marginX = comp.bounds.width * PAN_MARGIN_FACTOR;
        float marginY = comp.bounds.height * PAN_MARGIN_FACTOR;

        float halfPanX =
            std::abs(textureRect.w - comp.bounds.width) / 2.0f +
            marginX;

        float halfPanY =
            std::abs(textureRect.h - comp.bounds.height) / 2.0f +
            marginY;

        comp.offset.x =
            common::math::Clamp(comp.offset.x, -halfPanX, halfPanX);

        comp.offset.y =
            common::math::Clamp(comp.offset.y, -halfPanY, halfPanY);
    }

    void UIDrawingCanvas::draw(ClaySdL3RendererData *data)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        Clay_ElementData elementData = Clay_GetElementData(CLAY_ID("DrawingCanvas"));
        if (elementData.found) {
            comp.bounds = elementData.boundingBox;
        }

        auto mousePosInScreen = core::InputManager::GetMousePosition();
        auto mousePosInCanvas = screenToCanvas(mousePosInScreen);
        if (Clay_Hovered()) {
            /** Zoom */
            {
                auto [scrollDelta, deltaTime] = core::InputManager::GetScrollDelta("ui_scroll");
                if (scrollDelta.y != 0.0f) {
                    float oldZoom = comp.zoom;
                    comp.zoom = common::math::Clamp(comp.zoom + scrollDelta.y * 0.1f, 0.5f, 10.0f);

                    float relativeX = (mousePosInScreen.x - comp.bounds.x) / comp.bounds.width;
                    float relativeY = (mousePosInScreen.y - comp.bounds.y) / comp.bounds.height;

                    comp.offset.x += (comp.bounds.width / 2.0f - (relativeX - 0.5f) * comp.bounds.width) * (comp.zoom - oldZoom);
                    comp.offset.y += (comp.bounds.height / 2.0f - (relativeY - 0.5f) * comp.bounds.height) * (comp.zoom - oldZoom);

                    clampOffset(comp);

                    if (comp.zoom == 1.0f)
                        comp.offset = {0.0f, 0.0f};
                }
            }

            /** Pan */
            {
                if (core::InputManager::IsJustPressed("ui_rightClick")) {
                    comp.panning = true;
                    comp.lastPanMousePos = mousePosInScreen;
                }

                if (core::InputManager::IsPressed("ui_rightClick") && comp.panning) {
                    float deltaX = mousePosInScreen.x - comp.lastPanMousePos.x;
                    float deltaY = mousePosInScreen.y - comp.lastPanMousePos.y;

                    comp.offset.x += deltaX;
                    comp.offset.y += deltaY;

                    clampOffset(comp);

                    comp.lastPanMousePos = mousePosInScreen;
                }

                if (core::InputManager::IsReleased("ui_rightClick"))
                    comp.panning = false;
            }

            /** Dessin */
            {
                if (isInsideTextureRect(mousePosInScreen, comp)) {
                    atmo::core::types::Color paintColor = atmo::core::types::Color::BLACK;
                    if (core::InputManager::IsJustPressed("ui_click")) {
                        paintPixel(mousePosInCanvas, paintColor);
                    } else if (core::InputManager::IsPressed("ui_click")) {
                        if (mousePosInCanvas.x != comp.lastMousePos.x ||
                            mousePosInCanvas.y != comp.lastMousePos.y) {
                            paintLine(comp.lastMousePos, mousePosInCanvas, paintColor);
                        }
                    }
                }
            }
        }

        comp.lastMousePos = mousePosInCanvas;

        if (!comp.render_target)
            return;

        auto windowEntity = getWindow();
        if (!windowEntity)
            return;

        auto window = windowEntity->getComponentMutable<core::components::Window>();
        if (!window.renderer_data.renderer)
            return;

        SDL_Renderer *renderer = window.renderer_data.renderer;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_FRect canvasRect = { comp.bounds.x, comp.bounds.y, comp.bounds.width, comp.bounds.height };
        SDL_RenderFillRect(renderer, &canvasRect);

        SDL_Rect clipRect = {
            (int)comp.bounds.x,
            (int)comp.bounds.y,
            (int)comp.bounds.width,
            (int)comp.bounds.height
        };
        SDL_SetRenderClipRect(renderer, &clipRect);

        SDL_FRect textureRect = computeTextureRect(comp);
        SDL_RenderTexture(renderer, comp.render_target, nullptr, &textureRect);

        SDL_SetRenderClipRect(renderer, nullptr);
    }

    atmo::core::types::Vector2i UIDrawingCanvas::screenToCanvas(atmo::core::types::Vector2 screenPos) const
    {
        auto &comp = getComponent<components::UIDrawingCanvas>();
        SDL_FRect textureRect = computeTextureRect(comp);

        if (textureRect.w == 0 || textureRect.h == 0)
            return {0, 0};

        float relativeX = (screenPos.x - textureRect.x) / textureRect.w;
        float relativeY = (screenPos.y - textureRect.y) / textureRect.h;

        int textureX = (int)(relativeX * comp.textureSize.x);
        int textureY = (int)(relativeY * comp.textureSize.y);

        return {textureX, textureY};
    }

    atmo::core::types::Vector2 UIDrawingCanvas::canvasToScreen(atmo::core::types::Vector2i canvasPos) const
    {
        auto &comp = getComponent<components::UIDrawingCanvas>();
        SDL_FRect textureRect = computeTextureRect(comp);

        return {
            textureRect.x + (canvasPos.x / comp.textureSize.x) * textureRect.w,
            textureRect.y + (canvasPos.y / comp.textureSize.y) * textureRect.h
        };
    }

    void UIDrawingCanvas::paintLine(atmo::core::types::Vector2i from, atmo::core::types::Vector2i to, atmo::core::types::Color color)
    {
        int x0 = from.x, y0 = from.y;
        int x1 = to.x,   y1 = to.y;

        int deltaX = std::abs(x1 - x0);
        int deltaY = std::abs(y1 - y0);
        int stepX = x0 < x1 ? 1 : -1;
        int stepY = y0 < y1 ? 1 : -1;
        int accumulatedError = deltaX - deltaY;

        while (x0 != x1 || y0 != y1) {
            paintPixel({x0, y0}, color);
            int doubledError = 2 * accumulatedError;
            if (doubledError > -deltaY) { accumulatedError -= deltaY; x0 += stepX; }
            if (doubledError <  deltaX) { accumulatedError += deltaX; y0 += stepY; }
        }
        paintPixel({x1, y1}, color);
    }

    void UIDrawingCanvas::paintPixel(atmo::core::types::Vector2i pos, atmo::core::types::Color color)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();
        if (!comp.render_target) return;

        auto windowEntity = getWindow();
        if (!windowEntity) return;

        auto window = windowEntity->getComponentMutable<core::components::Window>();
        if (!window.renderer_data.renderer) return;

        SDL_Renderer *renderer = window.renderer_data.renderer;

        SDL_SetRenderTarget(renderer, comp.render_target);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, color.r * 255, color.g * 255, color.b * 255, color.a * 255);
        SDL_RenderPoint(renderer, (float)pos.x, (float)pos.y);
        SDL_SetRenderTarget(renderer, nullptr);
    }
}

ATMO_REGISTER_ENTITY(entities::UIDrawingCanvas);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIDrawingCanvas)
