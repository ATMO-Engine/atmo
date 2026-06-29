#include "ui_drawing_canvas.hpp"
#include "SDL3_image/SDL_image.h"
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

            if (comp.checkboard_texture) {
                SDL_DestroyTexture(comp.checkboard_texture);
                comp.checkboard_texture = nullptr;
            }
        });
    }

    void UIDrawingCanvas::initialize()
    {
        UI::initialize();
        setComponent<components::UIDrawingCanvas>({});
        createSignal<const core::types::Vector2i &>("New Dimensions");
    }

    Clay_ElementDeclaration UIDrawingCanvas::buildDecl()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        return Clay_ElementDeclaration{ .id = CLAY_ID("DrawingCanvas"),
                                        .layout = { .sizing = {
                                                        .width = CLAY_SIZING_PERCENT(comp.canvas_size.x),
                                                        .height = CLAY_SIZING_PERCENT(comp.canvas_size.y),
                                                    } } };
    }

    void UIDrawingCanvas::rebuildCheckboard()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        auto windowEntity = getWindow();
        if (!windowEntity) {
            return;
        }

        auto window = windowEntity->getComponentMutable<core::components::Window>();
        SDL_Renderer *renderer = window.renderer_data.renderer;
        if (!renderer) {
            return;
        }

        int w = comp.texture_size.x;
        int h = comp.texture_size.y;

        if (w <= 0 || h <= 0) {
            return;
        }

        if (comp.checkboard_texture) {
            SDL_DestroyTexture(comp.checkboard_texture);
        }

        comp.checkboard_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

        if (!comp.checkboard_texture) {
            spdlog::warn("Check board load failed");
            return;
        }

        SDL_SetTextureScaleMode(comp.checkboard_texture, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderTarget(renderer, comp.checkboard_texture);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                bool dark = ((x + y) % 2) == 0;
                if (dark)
                    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
                else
                    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
                SDL_RenderPoint(renderer, (float)x, (float)y);
            }
        }

        SDL_SetRenderTarget(renderer, nullptr);
    }

    void UIDrawingCanvas::validateAndSyncDimensions()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        comp.texture_size.x = std::max(1, comp.texture_size.x);
        comp.texture_size.y = std::max(1, comp.texture_size.y);

        int targetW = comp.texture_size.x;
        int targetH = comp.texture_size.y;

        int pixelH = (int)comp.pixels.size();
        int pixelW = pixelH > 0 ? (int)comp.pixels[0].size() : 0;

        const atmo::core::types::Color transparent{ 0.0f, 0.0f, 0.0f, 0.0f };

        if (pixelH != targetH || pixelW != targetW) {
            spdlog::warn("canvas sync: FrameBuffer: {}/{} != texture_size: {}/{}, syncing", pixelW, pixelH, targetW, targetH);
            comp.pixels.resize(targetH, std::vector<atmo::core::types::Color>(targetW, transparent));

            for (int y = 0; y < targetH; ++y) {
                auto &row = comp.pixels[y];
                row.resize(targetW, transparent);
            }
            comp.texture_dirty = true;
            rebuildCheckboard();
        }

        if (comp.drawing_texture) {
            float tw, th;
            SDL_GetTextureSize(comp.drawing_texture, &tw, &th);
            if ((int)tw != targetW || (int)th != targetH) {
                spdlog::warn("canvas sync: texture SDL: {}/{} != texture_size: {}/{}, destroy current SDL texture", (int)tw, (int)th, targetW, targetH);
                SDL_DestroyTexture(comp.drawing_texture);
                comp.drawing_texture = nullptr;
                comp.texture_dirty = true;
                rebuildCheckboard();
            }
        }
    }

    void UIDrawingCanvas::resizeCanvas(int width, int heigth)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        int oldH = (int)comp.pixels.size();
        int oldW = oldH > 0 ? (int)comp.pixels[0].size() : 0;

        if (heigth > oldH) {
            comp.pixels.resize(heigth, std::vector<atmo::core::types::Color>(width, { 0.0f, 0.0f, 0.0f, 0.0f }));
        } else {
            comp.pixels.resize(heigth, std::vector<atmo::core::types::Color>(width, { 0.0f, 0.0f, 0.0f, 0.0f }));
        }

        for (int y = 0; y < (int)comp.pixels.size(); ++y) {
            auto &row = comp.pixels[y];
            if (width > (int)row.size()) {
                row.resize(width, { 0.0f, 0.0f, 0.0f, 0.0f });
            } else {
                row.resize(width, { 0.0f, 0.0f, 0.0f, 0.0f });
            }
        }

        comp.texture_size = { width, heigth };

        if (comp.drawing_texture) {
            SDL_DestroyTexture(comp.drawing_texture);
            comp.drawing_texture = nullptr;
        }

        comp.texture_dirty = true;
        rebuildCheckboard();
    }

    void UIDrawingCanvas::initPixelBuffer(int w, int h)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        comp.pixels.assign(h, std::vector<atmo::core::types::Color>(w, atmo::core::types::Color{ 0.0f, 0.0f, 0.0f, 0.0f }));
        comp.texture_size = { w, h };

        if (comp.drawing_texture) {
            SDL_DestroyTexture(comp.drawing_texture);
            comp.drawing_texture = nullptr;
        }
        comp.texture_dirty = true;
        rebuildCheckboard();
        getSignal<const core::types::Vector2i &>("New Dimensions").emit(comp.texture_size);
    }

    atmo::core::types::Vector2 UIDrawingCanvas::screenToCanvas(atmo::core::types::Vector2 screenPos) const
    {
        auto &comp = getComponent<components::UIDrawingCanvas>();
        SDL_FRect textureRect = comp.cached_texture_rect;

        if (textureRect.w == 0 || textureRect.h == 0)
            return { 0, 0 };

        float relativeX = (screenPos.x - textureRect.x) / textureRect.w;
        float relativeY = (screenPos.y - textureRect.y) / textureRect.h;

        float textureX = relativeX * comp.texture_size.x;
        float textureY = relativeY * comp.texture_size.y;

        return { textureX, textureY };
    }

    atmo::core::types::Vector2 UIDrawingCanvas::canvasToScreen(atmo::core::types::Vector2i canvasPos) const
    {
        auto &comp = getComponent<components::UIDrawingCanvas>();
        SDL_FRect textureRect = comp.cached_texture_rect;

        return { textureRect.x + (canvasPos.x / comp.texture_size.x) * textureRect.w, textureRect.y + (canvasPos.y / comp.texture_size.y) * textureRect.h };
    }

    float UIDrawingCanvas::computeFitScale(const components::UIDrawingCanvas &comp) const
    {
        if (comp.texture_size.x <= 0 || comp.texture_size.y <= 0) {
            return 1;
        }

        float scaleX = comp.bounds.width / comp.texture_size.x;
        float scaleY = comp.bounds.height / comp.texture_size.y;

        return std::min(scaleX, scaleY);
    }

    SDL_FRect UIDrawingCanvas::computeTextureRect(const components::UIDrawingCanvas &comp) const
    {
        float scale = computeFitScale(comp) * comp.zoom;

        float drawW = comp.texture_size.x * scale;
        float drawH = comp.texture_size.y * scale;

        float drawX = comp.bounds.x + (comp.bounds.width - drawW) / 2.0f + comp.offset.x;
        float drawY = comp.bounds.y + (comp.bounds.height - drawH) / 2.0f + comp.offset.y;

        return { drawX, drawY, drawW, drawH };
    }

    bool UIDrawingCanvas::isInsideTextureRect(atmo::core::types::Vector2 screenPos, const components::UIDrawingCanvas &comp) const
    {
        SDL_FRect rect = comp.cached_texture_rect;
        return screenPos.x >= rect.x && screenPos.x < rect.x + rect.w && screenPos.y >= rect.y && screenPos.y < rect.y + rect.h;
    }

    void UIDrawingCanvas::clampOffset(components::UIDrawingCanvas &comp)
    {
        SDL_FRect textureRect = comp.cached_texture_rect;

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

            float relativeX = (mousePosInScreen.x - comp.cached_texture_rect.x) / comp.cached_texture_rect.w;
            float relativeY = (mousePosInScreen.y - comp.cached_texture_rect.y) / comp.cached_texture_rect.h;

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
            comp.last_pan_mouse_pos = mousePosInScreen;
        }

        if (core::InputManager::IsPressed("ui_rightClick") && comp.panning) {
            float deltaX = mousePosInScreen.x - comp.last_pan_mouse_pos.x;
            float deltaY = mousePosInScreen.y - comp.last_pan_mouse_pos.y;

            comp.offset.x += deltaX;
            comp.offset.y += deltaY;

            clampOffset(comp);

            comp.last_pan_mouse_pos = mousePosInScreen;
        }

        if (core::InputManager::IsReleased("ui_rightClick")) {
            comp.panning = false;
        }
    }

    void UIDrawingCanvas::draw(ClaySdL3RendererData *data)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        Clay_ElementData elementData = Clay_GetElementData(CLAY_ID("DrawingCanvas"));
        if (elementData.found) {
            comp.bounds = elementData.boundingBox;
        }

        comp.cached_texture_rect = computeTextureRect(comp);
        auto mousePosInScreen = core::InputManager::GetMousePosition();
        auto mousePosInCanvas = screenToCanvas(mousePosInScreen);
        if (Clay_Hovered()) {
            handleZoom(mousePosInScreen);
            handlePan(mousePosInScreen);
            handleDrawing(mousePosInScreen, mousePosInCanvas);
        } else {
            comp.last_paint_mouse_pos = mousePosInCanvas;
        }

        render();
    }

    void UIDrawingCanvas::render()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();


        auto windowEntity = getWindow();
        if (!windowEntity) {
            return;
        }
        auto window = windowEntity->getComponentMutable<core::components::Window>();
        SDL_Renderer *renderer = window.renderer_data.renderer;
        if (!renderer) {
            return;
        }
        if (comp.texture_dirty) {
            flushPixelsToTexture(renderer);
        }

        if (!comp.checkboard_texture) {
            spdlog::warn("CheckBoard texture null rebuild");
            rebuildCheckboard();
        }
        if (!comp.drawing_texture || !comp.checkboard_texture) {
            return;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_FRect canvasRect = { comp.bounds.x, comp.bounds.y, comp.bounds.width, comp.bounds.height };
        SDL_RenderFillRect(renderer, &canvasRect);

        SDL_Rect clipRect = { (int)comp.bounds.x, (int)comp.bounds.y, (int)comp.bounds.width, (int)comp.bounds.height };
        SDL_SetRenderClipRect(renderer, &clipRect);
        SDL_RenderTexture(renderer, comp.checkboard_texture, nullptr, &comp.cached_texture_rect);
        SDL_RenderTexture(renderer, comp.drawing_texture, nullptr, &comp.cached_texture_rect);
        SDL_SetRenderClipRect(renderer, nullptr);
    }

    void UIDrawingCanvas::flushPixelsToTexture(SDL_Renderer *renderer)
    {
        validateAndSyncDimensions();
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (!comp.texture_dirty)
            return;

        int h = (int)comp.pixels.size();
        int w = h > 0 ? (int)comp.pixels[0].size() : 0;

        if (w <= 0 || h <= 0)
            return;

        if (!comp.drawing_texture) {
            comp.drawing_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
            if (!comp.drawing_texture)
                return;
            SDL_SetTextureScaleMode(comp.drawing_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetTextureBlendMode(comp.drawing_texture, SDL_BLENDMODE_BLEND);
        }

        comp.upload_buffer.resize(w * h * 4);
        for (int y = 0; y < h; ++y) {
            const auto &row = comp.pixels[y];
            for (int x = 0; x < w; ++x) {
                const auto &c = row[x];
                int idx = (y * w + x) * 4;
                comp.upload_buffer[idx + 0] = (uint8_t)(c.r * 255.0f);
                comp.upload_buffer[idx + 1] = (uint8_t)(c.g * 255.0f);
                comp.upload_buffer[idx + 2] = (uint8_t)(c.b * 255.0f);
                comp.upload_buffer[idx + 3] = (uint8_t)(c.a * 255.0f);
            }
        }

        SDL_UpdateTexture(comp.drawing_texture, nullptr, comp.upload_buffer.data(), w * 4);
        comp.texture_dirty = false;
    }

    void UIDrawingCanvas::paintCapsule(
        const atmo::core::types::Vector2 &from, const atmo::core::types::Vector2 &to, int brush_radius, const atmo::core::types::Color &color)
    {
        float radius = brush_radius * 0.5f;

        int minX = (int)std::floor(std::min(from.x, to.x) - radius);
        int maxX = (int)std::ceil(std::max(from.x, to.x) + radius);
        int minY = (int)std::floor(std::min(from.y, to.y) - radius);
        int maxY = (int)std::ceil(std::max(from.y, to.y) + radius);

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
        validateAndSyncDimensions();

        auto &comp = getComponentMutable<components::UIDrawingCanvas>();
        int h = (int)comp.pixels.size();
        int w = h > 0 ? (int)comp.pixels[0].size() : 0;

        if (pos.x < 0 || pos.y < 0 || pos.x >= w || pos.y >= h) {
            return;
        }

        uint32_t index = pos.y * w + pos.x;
        if (!comp.painted_pixels.insert(index).second) {
            return;
        }

        auto &dst = comp.pixels[pos.y][pos.x];

        float srcA = color.a;
        float dstA = dst.a;
        float outA = srcA + dstA * (1.0f - srcA);

        if (outA > 0.0f) {
            dst.r = (color.r * srcA + dst.r * dstA * (1.0f - srcA)) / outA;
            dst.g = (color.g * srcA + dst.g * dstA * (1.0f - srcA)) / outA;
            dst.b = (color.b * srcA + dst.b * dstA * (1.0f - srcA)) / outA;
            dst.a = outA;
        } else {
            dst = atmo::core::types::Color{ 0.0f, 0.0f, 0.0f, 0.0f };
        }

        comp.texture_dirty = true;
    }

    void UIDrawingCanvas::handleDrawing(const atmo::core::types::Vector2 &mousePosInScreen, const atmo::core::types::Vector2 &mousePosInCanvas)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (isInsideTextureRect(mousePosInScreen, comp)) {
            if (core::InputManager::IsJustPressed("ui_click")) {
                comp.last_paint_mouse_pos = mousePosInCanvas;
                ++comp.current_strokeId;
                comp.painted_pixels.clear();
                paintCapsule(mousePosInCanvas, mousePosInCanvas, comp.brush_radius, comp.brush_color);
            } else if (core::InputManager::IsPressed("ui_click")) {
                if (mousePosInCanvas.x != comp.last_paint_mouse_pos.x || mousePosInCanvas.y != comp.last_paint_mouse_pos.y) {
                    paintCapsule(comp.last_paint_mouse_pos, mousePosInCanvas, comp.brush_radius, comp.brush_color);
                    comp.last_paint_mouse_pos = mousePosInCanvas;
                }
            }
        }
    }

    void UIDrawingCanvas::exportCanvas(const std::string &path)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();
        int w = comp.texture_size.x;
        int h = comp.texture_size.y;
        if (w <= 0 || h <= 0 || comp.pixels.empty())
            return;

        SDL_Surface *surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
        if (!surface)
            return;

        uint8_t *px = (uint8_t *)surface->pixels;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                const auto &c = comp.pixels[y][x];
                int i = y * surface->pitch + x * 4;
                px[i + 0] = (uint8_t)(c.r * 255.0f);
                px[i + 1] = (uint8_t)(c.g * 255.0f);
                px[i + 2] = (uint8_t)(c.b * 255.0f);
                px[i + 3] = (uint8_t)(c.a * 255.0f);
            }

        switch (comp.format) {
            case core::components::UIDrawingCanvas::ExportFormat::PNG:
                IMG_SavePNG(surface, path.c_str());
                break;
            case core::components::UIDrawingCanvas::ExportFormat::BMP:
                SDL_SaveBMP(surface, path.c_str());
                break;
            case core::components::UIDrawingCanvas::ExportFormat::JPG:
                IMG_SaveJPG(surface, path.c_str(), 90); // qualité 90
                break;
        }
        SDL_DestroySurface(surface);
    }

    void UIDrawingCanvas::importCanvas(const std::string &path)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        auto windowEntity = getWindow();
        if (!windowEntity)
            return;

        auto window = windowEntity->getComponentMutable<core::components::Window>();
        SDL_Renderer *renderer = window.renderer_data.renderer;
        if (!renderer)
            return;

        std::string ext = path.substr(path.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        SDL_Surface *surface = nullptr;

        if (ext == "bmp") {
            comp.format = core::components::UIDrawingCanvas::ExportFormat::BMP;
            surface = SDL_LoadBMP(path.c_str());
        } else if (ext == "png") {
            comp.format = core::components::UIDrawingCanvas::ExportFormat::PNG;
            surface = IMG_Load(path.c_str());
        } else if (ext == "jpg" || ext == "jpeg") {
            comp.format = core::components::UIDrawingCanvas::ExportFormat::JPG;
            surface = IMG_Load(path.c_str());
        }

        if (!surface)
            return;

        SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!rgba)
            return;

        int w = rgba->w;
        int h = rgba->h;

        comp.pixels.assign(h, std::vector<atmo::core::types::Color>(w, atmo::core::types::Color{ 0.0f, 0.0f, 0.0f, 0.0f }));
        uint8_t *px = (uint8_t *)rgba->pixels;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                int i = y * rgba->pitch + x * 4;
                comp.pixels[y][x] = { px[i + 0] / 255.0f, px[i + 1] / 255.0f, px[i + 2] / 255.0f, px[i + 3] / 255.0f };
            }
        SDL_DestroySurface(rgba);

        if (comp.drawing_texture) {
            SDL_DestroyTexture(comp.drawing_texture);
            comp.drawing_texture = nullptr;
        }

        comp.texture_size = { w, h };
        getSignal<const core::types::Vector2i &>("New Dimensions").emit(comp.texture_size);
        comp.zoom = 1.0f;
        comp.offset = { 0.0f, 0.0f };
        comp.texture_dirty = true;
        rebuildCheckboard();
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIDrawingCanvas);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIDrawingCanvas)
