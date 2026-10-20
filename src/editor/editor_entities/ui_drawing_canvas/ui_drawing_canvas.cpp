#include "ui_drawing_canvas.hpp"
#include "SDL3_image/SDL_image.h"
#include "common/math.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/input/input_manager.hpp"
#include "meta/auto_register.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::core::ecs::entities
{
    namespace
    {
        class DrawingCanvasRenderable : public ClayCustomRenderable
        {
        public:
            explicit DrawingCanvasRenderable(flecs::entity handle) : m_handle(handle) {}

            void render(server::rendering::CanvasCommandWriter &writer, const server::rendering::Rect2 &rect, float) override
            {
                if (m_handle.is_alive() && m_handle.has<components::UIDrawingCanvas>())
                    UIDrawingCanvas(m_handle).render(writer, rect);
            }

        private:
            flecs::entity m_handle;
        };
    } // namespace

    void UIDrawingCanvas::RegisterSystems(flecs::world *world)
    {
        world->observer<components::UIDrawingCanvas>().event(flecs::OnRemove).each([](flecs::entity e, components::UIDrawingCanvas &comp) {
            auto &server = server::rendering::RenderingServer::Instance();
            server.textureFree(comp.drawing_texture);
            server.textureFree(comp.checkboard_texture);
            comp.drawing_texture = {};
            comp.checkboard_texture = {};
            comp.m_renderable.reset();
        });
    }

    void UIDrawingCanvas::initialize()
    {
        UI::initialize();
        setComponent<components::UIDrawingCanvas>({});
        getComponentMutable<components::UIDrawingCanvas>().m_renderable = std::make_shared<DrawingCanvasRenderable>(p_handle);
        createSignal<const core::types::Vector2i &>("New Dimensions");
    }

    void UIDrawingCanvas::render(server::rendering::CanvasCommandWriter &writer, const server::rendering::Rect2 &rect)
    {
        const auto &comp = getComponent<components::UIDrawingCanvas>();
        if (rect.size.x <= 0.0f || rect.size.y <= 0.0f)
            return;

        const server::rendering::Rect2 texture_rect{ { comp.cached_texture_rect.x, comp.cached_texture_rect.y },
                                                     { comp.cached_texture_rect.w, comp.cached_texture_rect.h } };
        writer.pushClip(rect);
        if (!comp.preview) {
            writer.addRect(rect, core::types::Color(64.0f / 255.0f, 64.0f / 255.0f, 64.0f / 255.0f, 1.0f));
            if (comp.checkboard_texture.isValid())
                writer.addTextureRect(
                    texture_rect,
                    comp.checkboard_texture,
                    {
                        .source =
                            server::rendering::Rect2{ { 0.0f, 0.0f }, { static_cast<float>(comp.texture_size.x), static_cast<float>(comp.texture_size.y) } },
                        .filter = server::rendering::TextureFilter::Nearest,
                        .repeat = true,
                    });
        }
        if (comp.drawing_texture.isValid())
            writer.addTextureRect(texture_rect, comp.drawing_texture, { .filter = server::rendering::TextureFilter::Nearest });
        writer.popClip();
    }

    Clay_ElementDeclaration UIDrawingCanvas::buildDecl()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (server::rendering::RenderingServer::Instance().isInitialized()) {
            if (comp.texture_dirty)
                flushPixelsToTexture();
            if (!comp.checkboard_texture.isValid())
                rebuildCheckboard();
        }

        Clay_ElementDeclaration d{ .id = CLAY_ID("DrawingCanvas"),
                                   .layout = { .sizing = {
                                                   .width = CLAY_SIZING_PERCENT(comp.canvas_size.x),
                                                   .height = CLAY_SIZING_PERCENT(comp.canvas_size.y),
                                               } } };
        d.custom.customData = comp.m_renderable.get();

        return d;
    }

    void UIDrawingCanvas::rebuildCheckboard()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();
        auto &server = server::rendering::RenderingServer::Instance();
        if (comp.checkboard_texture.isValid() || !server.isInitialized())
            return;

        const std::byte dark{ 180 };
        const std::byte light{ 230 };
        const std::byte opaque{ 255 };
        const std::array<std::byte, 16> pixels{ dark, dark, dark, opaque, light, light, light, opaque, light, light, light, opaque, dark, dark, dark, opaque };
        comp.checkboard_texture = server.texture2dCreate({ .size = { 2, 2 }, .debug_name = "drawing canvas checkerboard" }, pixels);
        if (!comp.checkboard_texture.isValid())
            spdlog::warn("Check board load failed");
    }

    void UIDrawingCanvas::releaseDrawingTexture()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();
        server::rendering::RenderingServer::Instance().textureFree(comp.drawing_texture);
        comp.drawing_texture = {};
        comp.drawing_texture_size = { 0, 0 };
        comp.texture_dirty = true;
    }

    void UIDrawingCanvas::validateAndSyncDimensions()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        comp.texture_size.x = std::max(1, comp.texture_size.x);
        comp.texture_size.y = std::max(1, comp.texture_size.y);

        int targetW = comp.texture_size.x;
        int targetH = comp.texture_size.y;

        int pixelW = comp.image.width();
        int pixelH = comp.image.height();

        if (!comp.image.currentFrame()) {
            spdlog::warn("canvas sync: no frame, creating a blank {}/{} one", targetW, targetH);
            comp.image.reset(targetW, targetH);
            comp.texture_dirty = true;
            rebuildCheckboard();
        } else if (pixelH != targetH || pixelW != targetW) {
            spdlog::warn("canvas sync: FrameBuffer: {}/{} != texture_size: {}/{}, syncing", pixelW, pixelH, targetW, targetH);
            comp.image.resizeFrame(targetW, targetH);
            comp.texture_dirty = true;
            rebuildCheckboard();
        }

        if (comp.drawing_texture.isValid() && (comp.drawing_texture_size.x != targetW || comp.drawing_texture_size.y != targetH)) {
            spdlog::warn(
                "canvas sync: texture {}/{} != texture_size: {}/{}, recreating the texture",
                comp.drawing_texture_size.x,
                comp.drawing_texture_size.y,
                targetW,
                targetH);
            releaseDrawingTexture();
        }
    }

    void UIDrawingCanvas::resizeCanvas(int width, int heigth)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (comp.image.currentFrame())
            comp.image.resizeFrame(width, heigth);
        else
            comp.image.reset(width, heigth);

        comp.texture_size = { width, heigth };

        releaseDrawingTexture();

        comp.texture_dirty = true;
        rebuildCheckboard();
    }

    void UIDrawingCanvas::initPixelBuffer(int w, int h)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        comp.image.reset(w, h);
        comp.texture_size = { w, h };

        releaseDrawingTexture();
        comp.texture_dirty = true;
        rebuildCheckboard();
        getSignal<const core::types::Vector2i &>("New Dimensions").emit(comp.texture_size);
    }

    std::vector<std::vector<atmo::core::types::Color>> *UIDrawingCanvas::currentFrame()
    {
        auto *frame = getComponentMutable<components::UIDrawingCanvas>().image.currentFrame();
        return frame ? &frame->frame : nullptr;
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
        if (scrollDelta.y == 0.0f)
            return;

        float oldZoom = comp.zoom;
        float newZoom = common::math::Clamp(comp.zoom + scrollDelta.y * 0.1f, 0.5f, 10.0f);
        if (newZoom == oldZoom)
            return;

        SDL_FRect oldRect = comp.cached_texture_rect;
        if (oldRect.w <= 0.0f || oldRect.h <= 0.0f)
            return;

        float relativeX = (mousePosInScreen.x - oldRect.x) / oldRect.w;
        float relativeY = (mousePosInScreen.y - oldRect.y) / oldRect.h;

        comp.zoom = newZoom;

        float fitScale = computeFitScale(comp);
        float baseDrawW = comp.texture_size.x * fitScale;
        float baseDrawH = comp.texture_size.y * fitScale;

        comp.offset.x = mousePosInScreen.x - comp.bounds.x - (comp.bounds.width - baseDrawW * newZoom) / 2.0f - relativeX * baseDrawW * newZoom;
        comp.offset.y = mousePosInScreen.y - comp.bounds.y - (comp.bounds.height - baseDrawH * newZoom) / 2.0f - relativeY * baseDrawH * newZoom;

        clampOffset(comp);
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

    void UIDrawingCanvas::draw()
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
            if (!comp.preview) {
                handleZoom(mousePosInScreen);
                handlePan(mousePosInScreen);
            }
            if (comp.drawing_locked)
                comp.last_paint_mouse_pos = mousePosInCanvas;
            else
                handleDrawing(mousePosInScreen, mousePosInCanvas);
        } else {
            comp.last_paint_mouse_pos = mousePosInCanvas;
        }
    }

    void UIDrawingCanvas::flushPixelsToTexture()
    {
        validateAndSyncDimensions();
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (!comp.texture_dirty)
            return;

        const auto img = comp.image.renderAll();
        const auto *pixels = &img.frame;

        int h = (int)pixels->size();
        int w = h > 0 ? (int)(*pixels)[0].size() : 0;

        if (w <= 0 || h <= 0)
            return;

        auto &server = server::rendering::RenderingServer::Instance();
        if (!comp.drawing_texture.isValid()) {
            comp.drawing_texture = server.texture2dCreate({ .size = { w, h }, .debug_name = "drawing canvas" }, {});
            if (!comp.drawing_texture.isValid())
                return;
            comp.drawing_texture_size = { w, h };
        }

        comp.upload_buffer.resize(w * h * 4);
        for (int y = 0; y < h; ++y) {
            const auto &row = (*pixels)[y];
            for (int x = 0; x < w; ++x) {
                const auto &c = row[x];
                int idx = (y * w + x) * 4;
                comp.upload_buffer[idx + 0] = (uint8_t)(c.r * 255.0f);
                comp.upload_buffer[idx + 1] = (uint8_t)(c.g * 255.0f);
                comp.upload_buffer[idx + 2] = (uint8_t)(c.b * 255.0f);
                comp.upload_buffer[idx + 3] = (uint8_t)(c.a * 255.0f);
            }
        }

        server.texture2dUpdate(comp.drawing_texture, { .width = w, .height = h }, std::as_bytes(std::span(comp.upload_buffer)));
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
                float pixelCenterX = x + 0.5f;
                float pixelCenterY = y + 0.5f;

                float t = 0.0f;

                if (segmentLengthSquared > 0.0f) {
                    t = ((pixelCenterX - from.x) * dx + (pixelCenterY - from.y) * dy) / segmentLengthSquared;
                    t = std::clamp(t, 0.0f, 1.0f);
                }

                float closestX = from.x + t * dx;
                float closestY = from.y + t * dy;

                float distX = pixelCenterX - closestX;
                float distY = pixelCenterY - closestY;

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
        auto *pixels = currentFrame();
        if (!pixels)
            return;

        int h = (int)pixels->size();
        int w = h > 0 ? (int)(*pixels)[0].size() : 0;

        if (pos.x < 0 || pos.y < 0 || pos.x >= w || pos.y >= h) {
            return;
        }

        uint32_t index = pos.y * w + pos.x;
        if (!comp.painted_pixels.insert(index).second) {
            return;
        }

        auto &dst = (*pixels)[pos.y][pos.x];

        if (comp.pen == components::UIDrawingCanvas::DrawType::ERASER) {
            dst = atmo::core::types::Color{ 0.0f, 0.0f, 0.0f, 0.0f };
        } else {
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
        }

        comp.texture_dirty = true;
    }

    void UIDrawingCanvas::handleDrawing(const atmo::core::types::Vector2 &mousePosInScreen, const atmo::core::types::Vector2 &mousePosInCanvas)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (isInsideTextureRect(mousePosInScreen, comp)) {
            if (core::InputManager::IsJustPressed("ui_click")) {
                comp.last_paint_mouse_pos = mousePosInCanvas;
                ++comp.current_stroke_id;
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

    void UIDrawingCanvas::saveCanvas()
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        std::string path = comp.file_path;

        if (path.empty()) {
            // TODO: pop up filePath
            spdlog::info("Save Canvas: File path empty, no op");
            return;
        }

        std::string ext = std::filesystem::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".atx")
            comp.image.save(path);
        else
            exportSpriteSheet(path, comp.image.layerNames());
    }

    SDL_Surface *UIDrawingCanvas::LoadRgbaSurface(const std::string &path)
    {
        std::string ext = path.substr(path.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != "bmp" && ext != "png" && ext != "jpg" && ext != "jpeg")
            throw ImportException(path, "file format not valid, file format available: .atx, .bmp, .png, .jpg");

        SDL_Surface *surface = IMG_Load(path.c_str());
        if (!surface)
            throw ImportException(path, SDL_GetError());

        SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!rgba)
            throw ImportException(path, SDL_GetError());
        return rgba;
    }

    components::UIDrawingCanvas::ExportFormat UIDrawingCanvas::FormatFromPath(const std::string &path)
    {
        std::string ext = path.substr(path.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == "bmp")
            return components::UIDrawingCanvas::ExportFormat::BMP;
        if (ext == "jpg" || ext == "jpeg")
            return components::UIDrawingCanvas::ExportFormat::JPG;
        return components::UIDrawingCanvas::ExportFormat::PNG;
    }

    bool UIDrawingCanvas::exportSpriteSheet(const std::string &path, const std::vector<std::string> &layerNames)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        const auto frames = comp.image.renderLayers(layerNames);
        const int frameW = comp.image.width();
        const int frameH = comp.image.height();
        if (frames.empty() || frameW <= 0 || frameH <= 0) {
            spdlog::warn("Export sprite sheet: nothing to export");
            return false;
        }

        SDL_Surface *surface = SDL_CreateSurface(frameW * static_cast<int>(frames.size()), frameH, SDL_PIXELFORMAT_RGBA32);
        if (!surface) {
            spdlog::error("Export sprite sheet: {}", SDL_GetError());
            return false;
        }

        uint8_t *px = (uint8_t *)surface->pixels;
        for (std::size_t f = 0; f < frames.size(); ++f) {
            const int originX = static_cast<int>(f) * frameW;
            for (int y = 0; y < frameH && y < (int)frames[f].frame.size(); ++y)
                for (int x = 0; x < frameW && x < (int)frames[f].frame[y].size(); ++x) {
                    const auto &c = frames[f].frame[y][x];
                    int i = y * surface->pitch + (originX + x) * 4;
                    px[i + 0] = (uint8_t)(c.r * 255.0f);
                    px[i + 1] = (uint8_t)(c.g * 255.0f);
                    px[i + 2] = (uint8_t)(c.b * 255.0f);
                    px[i + 3] = (uint8_t)(c.a * 255.0f);
                }
        }

        bool saved = false;
        switch (FormatFromPath(path)) {
            case core::components::UIDrawingCanvas::ExportFormat::PNG:
                saved = IMG_SavePNG(surface, path.c_str());
                break;
            case core::components::UIDrawingCanvas::ExportFormat::BMP:
                saved = SDL_SaveBMP(surface, path.c_str());
                break;
            case core::components::UIDrawingCanvas::ExportFormat::JPG:
                saved = IMG_SaveJPG(surface, path.c_str(), 90);
                break;
        }
        if (!saved)
            spdlog::error(R"(Export sprite sheet "{}": {})", path, SDL_GetError());

        SDL_DestroySurface(surface);
        return saved;
    }

    void UIDrawingCanvas::importCanvas(const std::string &path)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (path.empty()) {
            spdlog::info("Import Canvas: File path empty, no op");
            return;
        }

        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::exists(path, ec) || !fs::is_regular_file(path, ec)) {
            spdlog::info("Import Canvas: the file {} doesn't exist", path);
            return;
        }

        std::string ext = path.substr(path.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        /* temporary check. Must be reworked */
        auto fileSize = fs::file_size(path, ec);
        if ((!ec && fileSize == 0) && (ext == "bmp" || ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "atmo")) {
            spdlog::info("Import Canvas: file {} exists but is empty, creating blank 16x16 canvas", path);
            initPixelBuffer(DEFAULT_TEXTURE_SIZE, DEFAULT_TEXTURE_SIZE);
            comp.file_path = path;
            return;
        }

        if (ext == "atmo") {
            atmo::image::extension::AtmoFormat loaded;
            if (!loaded.load(path))
                return;
            if (loaded.width() == 0 || loaded.height() == 0) {
                spdlog::warn("Import Canvas: {} has an empty frame size, no op", path);
                return;
            }

            comp.image = std::move(loaded);
            comp.file_path = path;
            imageReloaded(comp.image.width(), comp.image.height());
            return;
        }

        try {
            auto size = ProbeImageSize(path);
            importSpriteSheet(path, size.x, size.y, 1);
        } catch (const ImportException &e) {
            spdlog::warn("Import Canvas: {}, no op", e.what());
        }
    }

    atmo::core::types::Vector2i UIDrawingCanvas::ProbeImageSize(const std::string &path)
    {
        SDL_Surface *surface = LoadRgbaSurface(path);
        atmo::core::types::Vector2i size = { surface->w, surface->h };
        SDL_DestroySurface(surface);
        return size;
    }

    void UIDrawingCanvas::importSpriteSheet(const std::string &path, int frameW, int frameH, int frameCount)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        if (frameW < 1 || frameW > MAX_FRAME_SIZE || frameH < 1 || frameH > MAX_FRAME_SIZE)
            throw ImportException(path, std::format("frame size {}x{} not within 1-{} px range", frameW, frameH, MAX_FRAME_SIZE));
        if (frameCount < 1 || frameCount > UINT8_MAX)
            throw ImportException(path, std::format("frame count {} not within 1-{} range", frameCount, UINT8_MAX));

        SDL_Surface *rgba = LoadRgbaSurface(path);
        const int sheetW = rgba->w;
        const int sheetH = rgba->h;
        const int columns = sheetW / frameW;
        const int rows = sheetH / frameH;
        if (frameCount > columns * rows) {
            SDL_DestroySurface(rgba);
            throw ImportException(
                path, std::format("a {}x{} sheet only holds {} frames of {}x{}, {} requested", sheetW, sheetH, columns * rows, frameW, frameH, frameCount));
        }

        comp.image.reset(frameW, frameH);
        comp.image.setFrameNumber(static_cast<std::uint8_t>(frameCount));

        const uint8_t *px = (const uint8_t *)rgba->pixels;
        for (int f = 0; f < frameCount; ++f) {
            comp.image.selectFrame(static_cast<std::uint8_t>(f));
            auto &pixels = *currentFrame();
            const int originX = (f % columns) * frameW;
            const int originY = (f / columns) * frameH;

            for (int y = 0; y < frameH; ++y)
                for (int x = 0; x < frameW; ++x) {
                    int i = (originY + y) * rgba->pitch + (originX + x) * 4;
                    pixels[y][x] = { px[i + 0] / 255.0f, px[i + 1] / 255.0f, px[i + 2] / 255.0f, px[i + 3] / 255.0f };
                }
        }
        SDL_DestroySurface(rgba);
        comp.image.selectFrame(0);

        comp.format = FormatFromPath(path);
        comp.file_path = path;
        imageReloaded(frameW, frameH);
    }

    void UIDrawingCanvas::imageReloaded(int w, int h)
    {
        auto &comp = getComponentMutable<components::UIDrawingCanvas>();

        releaseDrawingTexture();

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
