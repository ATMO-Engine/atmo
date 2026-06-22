#include "texture_editor.hpp"
#include "editor/editor_registry.hpp"

#include "core/ecs/entities/ui/ui_drawing_canvas/ui_drawing_canvas.hpp"

namespace atmo::editor
{
    void TextureEditor::init(atmo::core::ecs::entities::UI &container)
    {
        auto canvas = core::ecs::EntityRegistry::Create<core::ecs::entities::UIDrawingCanvas>("Entity::UI::UIDrawingCanvas");
        auto &canvasInfo = canvas->getComponentMutable<core::components::UIDrawingCanvas>();

        auto windowEntity = container.getWindow();
        auto &windowComp = windowEntity->getComponentMutable<core::components::Window>();
        if (!windowComp.renderer_data.renderer) {
            return;
        }

        canvasInfo.textureSize = { 128, 80 };
        canvasInfo.canvasSize = { 0.40f, 0.30f };
        canvasInfo.render_target = SDL_CreateTexture(
            windowComp.renderer_data.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, canvasInfo.textureSize.x, canvasInfo.textureSize.y);
        SDL_SetTextureScaleMode(canvasInfo.render_target, SDL_SCALEMODE_NEAREST);

        SDL_SetRenderTarget(windowComp.renderer_data.renderer, canvasInfo.render_target);
        SDL_SetRenderDrawColor(windowComp.renderer_data.renderer, 255, 255, 255, 255);
        SDL_RenderClear(windowComp.renderer_data.renderer);
        SDL_SetRenderTarget(windowComp.renderer_data.renderer, nullptr);

        canvasInfo.zoom = 1.0f;
        canvasInfo.offset = { 0.0f, 0.0f };

        canvas->setParent(container);
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::TextureEditor);
