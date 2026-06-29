#pragma once

#include <unordered_set>
#include "SDL3/SDL.h"
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/types.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{

    struct UIDrawingCanvas {
        enum class ExportFormat {
            PNG,
            BMP,
            JPG
        };

        Clay_BoundingBox bounds = {};

        std::vector<std::vector<atmo::core::types::Color>> pixels;
        std::vector<uint8_t> upload_buffer;
        uint32_t current_strokeId = 0;
        std::unordered_set<uint32_t> painted_pixels;

        SDL_Texture *drawing_texture = nullptr;
        SDL_Texture *checkboard_texture = nullptr;
        bool texture_dirty = true;
        ExportFormat format = ExportFormat::PNG;

        atmo::core::types::Vector2 canvas_size = { 0.0f, 0.0f };
        atmo::core::types::Vector2i texture_size = { 1, 1 };

        SDL_FRect cached_texture_rect = {};
        float zoom = 1.0f;
        atmo::core::types::Vector2 offset = { 0.0f, 0.0f };
        atmo::core::types::Vector2 last_paint_mouse_pos = { 0.0f, 0.0f };
        atmo::core::types::Vector2 last_pan_mouse_pos = { 0.0f, 0.0f };
        bool panning = false;

        int brush_radius = 10;
        atmo::core::types::Color brush_color = atmo::core::types::Color::BLACK;
        float brush_spacing = 0.5f;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIDrawingCanvas> {
    static constexpr const char *name = "DrawingCanvas";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIDrawingCanvas::zoom>("zoom"),
        atmo::meta::field<&atmo::core::components::UIDrawingCanvas::offset>("offset"));
};

namespace atmo::core::ecs::entities
{
    constexpr float PAN_MARGIN_FACTOR = 0.25f;

    class UIDrawingCanvas : public EntityRegistry::Registrable<UIDrawingCanvas, UI>
    {
    public:
        using EntityRegistry::Registrable<UIDrawingCanvas, UI>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIDrawingCanvas";
        }

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;

        atmo::core::types::Vector2 screenToCanvas(atmo::core::types::Vector2 screenPos) const;
        atmo::core::types::Vector2 canvasToScreen(atmo::core::types::Vector2i canvasPos) const;

        void validateAndSyncDimensions();

        void exportCanvas(const std::string &path);
        void importCanvas(const std::string &path);
        void initPixelBuffer(int w, int h);

        void resizeCanvas(int width, int heigth);

    private:
        void rebuildCheckboard();

        void paintPixel(const atmo::core::types::Vector2i &pos, const atmo::core::types::Color &color);
        void
        paintCapsule(const atmo::core::types::Vector2 &from, const atmo::core::types::Vector2 &to, int brush_radius, const atmo::core::types::Color &color);

        float computeFitScale(const components::UIDrawingCanvas &comp) const;
        SDL_FRect computeTextureRect(const components::UIDrawingCanvas &comp) const;
        bool isInsideTextureRect(atmo::core::types::Vector2 screenPos, const components::UIDrawingCanvas &comp) const;

        void handleZoom(const atmo::core::types::Vector2 &mousePosInScreen);
        void handlePan(const atmo::core::types::Vector2 &mousePosInScreen);
        void handleDrawing(const atmo::core::types::Vector2 &mousePosInScreen, const atmo::core::types::Vector2 &mousePosInCanvas);
        void render();
        void flushPixelsToTexture(SDL_Renderer *renderer);

        void clampOffset(components::UIDrawingCanvas &comp);
    };
} // namespace atmo::core::ecs::entities
