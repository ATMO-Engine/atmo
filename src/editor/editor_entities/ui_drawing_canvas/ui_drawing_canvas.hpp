#pragma once

#include "SDL3/SDL.h"
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/types.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct UIDrawingCanvas {
        Clay_BoundingBox bounds = {};
        SDL_Texture *drawing_texture = nullptr;
        SDL_Texture *checkerboard_texture = nullptr;

        atmo::core::types::Vector2 canvasSize = { 0.0f, 0.0f };
        atmo::core::types::Vector2 textureSize = { 0.0f, 0.0f };

        SDL_FRect cachedTextureRect = {};
        float zoom = 1.0f;
        atmo::core::types::Vector2 offset = { 0.0f, 0.0f };
        atmo::core::types::Vector2i lastMousePos = { 0, 0 };
        atmo::core::types::Vector2 lastPanMousePos = { 0.0f, 0.0f };
        bool panning = false;
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

        atmo::core::types::Vector2i screenToCanvas(atmo::core::types::Vector2 screenPos) const;
        atmo::core::types::Vector2 canvasToScreen(atmo::core::types::Vector2i canvasPos) const;

    private:
        void paintPixel(const atmo::core::types::Vector2i &pos, const atmo::core::types::Color &color);
        void paintLine(const atmo::core::types::Vector2i &from, const atmo::core::types::Vector2i &to, const atmo::core::types::Color &color);

        float computeFitScale(const components::UIDrawingCanvas &comp) const;
        SDL_FRect computeTextureRect(const components::UIDrawingCanvas &comp) const;
        bool isInsideTextureRect(atmo::core::types::Vector2 screenPos, const components::UIDrawingCanvas &comp) const;

        void handleZoom(const atmo::core::types::Vector2 &mousePosInScreen);
        void handlePan(const atmo::core::types::Vector2 &mousePosInScreen);
        void handleDrawing(const atmo::core::types::Vector2 &mousePosInScreen, const atmo::core::types::Vector2i &mousePosInCanvas);
        void render();

        void clampOffset(components::UIDrawingCanvas &comp);
    };
} // namespace atmo::core::ecs::entities
