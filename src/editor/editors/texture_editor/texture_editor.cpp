#include "texture_editor.hpp"
#include "editor/editor_entities/ui_drawing_canvas/ui_drawing_canvas.hpp"
#include "editor/editor_registry.hpp"

namespace atmo::editor
{
    void TextureEditor::init(atmo::core::ecs::entities::UI &container)
    {
        auto canvas = core::ecs::EntityRegistry::Create<core::ecs::entities::UIDrawingCanvas>("Entity::UI::UIDrawingCanvas");
        auto &canvasInfo = canvas->getComponentMutable<core::components::UIDrawingCanvas>();
        canvasInfo.textureSize = { 128, 80 };
        canvasInfo.canvasSize = { 0.40f, 0.30f };
        canvasInfo.zoom = 1.0f;
        canvasInfo.offset = { 0.0f, 0.0f };

        auto windowEntity = container.getWindow();
        auto &windowComp = windowEntity->getComponentMutable<core::components::Window>();
        if (!windowComp.renderer_data.renderer) {
            return;
        }

        { /* Create the texture where the user draw */
            canvasInfo.drawing_texture = SDL_CreateTexture(
                windowComp.renderer_data.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, canvasInfo.textureSize.x, canvasInfo.textureSize.y);
            SDL_SetTextureScaleMode(canvasInfo.drawing_texture, SDL_SCALEMODE_NEAREST);

            SDL_SetRenderTarget(windowComp.renderer_data.renderer, canvasInfo.drawing_texture);
            SDL_SetRenderDrawColor(windowComp.renderer_data.renderer, 0, 0, 0, 0);
            SDL_RenderClear(windowComp.renderer_data.renderer);
            SDL_SetRenderTarget(windowComp.renderer_data.renderer, nullptr);
        }

        { /* Create a checkboard texture to render behind the drawing texture */
            canvasInfo.checkerboard_texture = SDL_CreateTexture(
                windowComp.renderer_data.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, canvasInfo.textureSize.x, canvasInfo.textureSize.y);
            SDL_SetTextureScaleMode(canvasInfo.checkerboard_texture, SDL_SCALEMODE_NEAREST);

            SDL_SetRenderTarget(windowComp.renderer_data.renderer, canvasInfo.checkerboard_texture);
            for (int y = 0; y < canvasInfo.textureSize.y; ++y) {
                for (int x = 0; x < canvasInfo.textureSize.x; ++x) {
                    bool dark = ((x + y) % 2) == 0;

                    if (dark) {
                        SDL_SetRenderDrawColor(windowComp.renderer_data.renderer, 180, 180, 180, 255);
                    } else {
                        SDL_SetRenderDrawColor(windowComp.renderer_data.renderer, 230, 230, 230, 255);
                    }

                    SDL_RenderPoint(windowComp.renderer_data.renderer, (float)x, (float)y);
                }
            }
            SDL_SetRenderTarget(windowComp.renderer_data.renderer, nullptr);
        }
        canvas->setParent(container);
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::TextureEditor);
