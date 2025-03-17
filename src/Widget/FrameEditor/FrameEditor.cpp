#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "imgui_impl_sdlrenderer3.h"

#include "FrameEditor.hpp"

FrameEditor::FrameEditor(SDL_Window *window)
    : _window(window)
{
    _renderer = SDL_CreateRenderer(_window, nullptr);

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_TARGET, _width, _height);

    // Clear canvas to white
    SDL_SetRenderTarget(_renderer, _texture);
    SDL_SetRenderDrawColor(_renderer, 255, 255, 255, 255);
    SDL_RenderClear(_renderer);
    SDL_SetRenderTarget(_renderer, NULL);

    // Set texture ID for ImGui
    _textureID = (ImTextureID)_texture;
}


void FrameEditor::draw()
{
    SDL_SetRenderTarget(_renderer, _texture);  // Set target to canvas texture
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);  // Black color for drawing

    for (size_t i = 1; i < points.size(); ++i) {
        SDL_RenderLine(_renderer, points[i - 1].x, points[i - 1].y, points[i].x, points[i].y);
    }

    SDL_SetRenderTarget(_renderer, NULL);
}

void FrameEditor::run()
{
    // Get cursor position inside ImGui
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImVec2(_width, _height);

    // Draw the texture as an image in ImGui
    ImGui::Image(_textureID, canvasSize);

    // Check for mouse input
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isDrawing = ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left);

    if (isDrawing) {
        int x = static_cast<int>(mousePos.x - canvasPos.x);
        int y = static_cast<int>(mousePos.y - canvasPos.y);

        if (x >= 0 && x < _width && y >= 0 && y < _height) {
            points.push_back({x, y});
        }
    }

    // Render lines to canvas texture
    draw();
}
