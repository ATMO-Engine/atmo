#include "ui_drawing_canvas.hpp"
#include "common/math.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/input/input_manager.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void UIDrawingCanvas::RegisterSystems(flecs::world *world)
    {
        world->observer<components::UIDrawingCanvas>().event(flecs::OnRemove).each([](flecs::entity e, components::UIDrawingCanvas &comp) {
            if (comp.drawing_texture) {
                SDL_DestroyTexture(comp.drawing_texture);
                comp.drawing_texture = nullptr;
            }

            if (comp.checkerboard_texture) {
                SDL_DestroyTexture(comp.checkerboard_texture);
                comp.checkerboard_texture = nullptr;
            }
        });
    }

    void UIDrawingCanvas::initialize()
    {
        UI::initialize();
        setComponent<components::UIDrawingCanvas>({});
        createSignal<UIDrawingCanvas &>("FetchBrush");
    }

    Clay_ElementDeclaration UIDrawingCanvas::buildDecl()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        return Clay_ElementDeclaration{ .id = CLAY_ID("DrawingCanvas"),
                                        .layout = { .sizing = {
                                                        .width = CLAY_SIZING_PERCENT(comp.canvasSize.x),
                                                        .height = CLAY_SIZING_PERCENT(comp.canvasSize.y),
                                                    } } };
    }

    atmo::core::types::Vector2i UIDrawingCanvas::screenToCanvas(atmo::core::types::Vector2 screenPos) const
    {
        auto &comp = getComponent<components::UIDrawingCanvas>();
        SDL_FRect textureRect = comp.cachedTextureRect;

        if (textureRect.w == 0 || textureRect.h == 0)
            return { 0, 0 };

        float relativeX = (screenPos.x - textureRect.x) / textureRect.w;
        float relativeY = (screenPos.y - textureRect.y) / textureRect.h;

        int textureX = (int)(relativeX * comp.textureSize.x);
        int textureY = (int)(relativeY * comp.textureSize.y);

        return { textureX, textureY };
    }

    atmo::core::types::Vector2 UIDrawingCanvas::canvasToScreen(atmo::core::types::Vector2i canvasPos) const
    {
        auto &comp = getComponent<components::UIDrawingCanvas>();
        SDL_FRect textureRect = comp.cachedTextureRect;

        return { textureRect.x + (canvasPos.x / comp.textureSize.x) * textureRect.w, textureRect.y + (canvasPos.y / comp.textureSize.y) * textureRect.h };
    }

    float UIDrawingCanvas::computeFitScale(const components::UIDrawingCanvas &comp) const
    {
        if (comp.textureSize.x <= 0 || comp.textureSize.y <= 0) {
            return 1;
        }

        float scaleX = comp.bounds.width / comp.textureSize.x;
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
        SDL_FRect rect = comp.cachedTextureRect;
        return screenPos.x >= rect.x && screenPos.x < rect.x + rect.w && screenPos.y >= rect.y && screenPos.y < rect.y + rect.h;
    }

    void UIDrawingCanvas::clampOffset(components::UIDrawingCanvas &comp)
    {
        SDL_FRect textureRect = comp.cachedTextureRect;

        float marginX = comp.bounds.width * PAN_MARGIN_FACTOR;
        float marginY = comp.bounds.height * PAN_MARGIN_FACTOR;

        float halfPanX = std::abs(textureRect.w - comp.bounds.width) / 2.0f + marginX;
        float halfPanY = std::abs(textureRect.h - comp.bounds.height) / 2.0f + marginY;

        comp.offset.x = common::math::Clamp(comp.offset.x, -halfPanX, halfPanX);
        comp.offset.y = common::math::Clamp(comp.offset.y, -halfPanY, halfPanY);
    }

    void UIDrawingCanvas::handleZoom(const atmo::core::types::Vector2 &mousePosInScreen)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        auto [scrollDelta, deltaTime] = core::InputManager::GetScrollDelta("ui_scroll");
        if (scrollDelta.y != 0.0f) {
            float oldZoom = comp.zoom;
            comp.zoom = common::math::Clamp(comp.zoom + scrollDelta.y * 0.1f, 0.5f, 10.0f);

            float relativeX = (mousePosInScreen.x - comp.cachedTextureRect.x) / comp.cachedTextureRect.w;
            float relativeY = (mousePosInScreen.y - comp.cachedTextureRect.y) / comp.cachedTextureRect.h;

            comp.offset.x += (comp.bounds.width / 2.0f - (relativeX - 0.5f) * comp.bounds.width) * (comp.zoom - oldZoom);
            comp.offset.y += (comp.bounds.height / 2.0f - (relativeY - 0.5f) * comp.bounds.height) * (comp.zoom - oldZoom);

            clampOffset(comp);
        }
    }

    void UIDrawingCanvas::handlePan(const atmo::core::types::Vector2 &mousePosInScreen)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

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

        if (core::InputManager::IsReleased("ui_rightClick")) {
            comp.panning = false;
        }
    }

    void UIDrawingCanvas::draw(ClaySdL3RendererData *data)
    {
        getSignal<UIDrawingCanvas &>("FetchBrush").emit(*this);

        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        Clay_ElementData elementData = Clay_GetElementData(CLAY_ID("DrawingCanvas"));
        if (elementData.found) {
            comp.bounds = elementData.boundingBox;
        }

        comp.cachedTextureRect = computeTextureRect(comp);
        auto mousePosInScreen = core::InputManager::GetMousePosition();
        auto mousePosInCanvas = screenToCanvas(mousePosInScreen);
        if (Clay_Hovered()) {
            handleZoom(mousePosInScreen);
            handlePan(mousePosInScreen);
            handleDrawing(mousePosInScreen, mousePosInCanvas);
        }
        comp.lastMousePos = mousePosInCanvas;

        render();
    }

    void UIDrawingCanvas::render()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (!comp.drawing_texture || !comp.checkerboard_texture) {
            return;
        }

        auto windowEntity = getWindow();
        if (!windowEntity) {
            return;
        }
        auto window = windowEntity->getComponentMutable<core::components::Window>();
        if (!window.renderer_data.renderer) {
            return;
        }
        SDL_Renderer *renderer = window.renderer_data.renderer;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_FRect canvasRect = { comp.bounds.x, comp.bounds.y, comp.bounds.width, comp.bounds.height };
        SDL_RenderFillRect(renderer, &canvasRect);

        SDL_Rect clipRect = { (int)comp.bounds.x, (int)comp.bounds.y, (int)comp.bounds.width, (int)comp.bounds.height };
        SDL_SetRenderClipRect(renderer, &clipRect);
        SDL_RenderTexture(renderer, comp.checkerboard_texture, nullptr, &comp.cachedTextureRect);
        SDL_RenderTexture(renderer, comp.drawing_texture, nullptr, &comp.cachedTextureRect);
        SDL_SetRenderClipRect(renderer, nullptr);
    }

    void UIDrawingCanvas::paintCapsule(
        const atmo::core::types::Vector2i &from, const atmo::core::types::Vector2i &to, int brushRadius, const atmo::core::types::Color &color)
    {
        int radius = (brushRadius - 1) * 0.5f;

        int minX = std::min(from.x, to.x) - radius;
        int maxX = std::max(from.x, to.x) + radius;

        int minY = std::min(from.y, to.y) - radius;
        int maxY = std::max(from.y, to.y) + radius;

        float dx = static_cast<float>(to.x - from.x);
        float dy = static_cast<float>(to.y - from.y);

        float segmentLengthSquared = dx * dx + dy * dy;
        float radiusSquared = static_cast<float>(radius * radius);

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float t = 0.0f;

                if (segmentLengthSquared > 0.0f) {
                    t = ((x - from.x) * dx + (y - from.y) * dy) / segmentLengthSquared;

                    t = std::clamp(t, 0.0f, 1.0f);
                }

                float closestX = from.x + t * dx;
                float closestY = from.y + t * dy;

                float distX = x - closestX;
                float distY = y - closestY;

                float distanceSquared = distX * distX + distY * distY;

                if (distanceSquared <= radiusSquared) {
                    paintPixel({ x, y }, color);
                }
            }
        }
    }

    void UIDrawingCanvas::paintPixel(const atmo::core::types::Vector2i &pos, const atmo::core::types::Color &color)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();
        if (!comp.drawing_texture)
            return;
        if (pos.x < 0 || pos.y < 0 || pos.x >= comp.textureSize.x || pos.y >= comp.textureSize.y) {
            return;
        }

        auto windowEntity = getWindow();
        if (!windowEntity)
            return;

        auto window = windowEntity->getComponentMutable<core::components::Window>();
        if (!window.renderer_data.renderer)
            return;

        SDL_Renderer *renderer = window.renderer_data.renderer;

        SDL_SetRenderTarget(renderer, comp.drawing_texture);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, color.r * 255, color.g * 255, color.b * 255, color.a * 255);
        SDL_RenderPoint(renderer, (float)pos.x, (float)pos.y);
        SDL_SetRenderTarget(renderer, nullptr);
    }

    void UIDrawingCanvas::handleDrawing(const atmo::core::types::Vector2 &mousePosInScreen, const atmo::core::types::Vector2i &mousePosInCanvas)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (isInsideTextureRect(mousePosInScreen, comp)) {
            if (core::InputManager::IsJustPressed("ui_click")) {
                paintCapsule(mousePosInCanvas, mousePosInCanvas, comp.brushRadius, comp.brushColor);
            } else if (core::InputManager::IsPressed("ui_click")) {
                if (mousePosInCanvas.x != comp.lastMousePos.x || mousePosInCanvas.y != comp.lastMousePos.y) {
                    paintCapsule(comp.lastMousePos, mousePosInCanvas, comp.brushRadius, comp.brushColor);
                }
            }
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIDrawingCanvas);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIDrawingCanvas)
