
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_stdinc.h"
#include "spdlog/spdlog.h"

#include <cstdint>
#include "FrameEditor.hpp"

FrameEditor::FrameEditor(SDL_Window *window) { _window = window; }

FrameEditor::~FrameEditor()
{
    if (_renderer) {
        SDL_DestroyRenderer(_renderer);
    }
    if (_texture) {
        SDL_DestroyTexture(_texture);
    }
}


GLuint FrameEditor::SDLTextureToOpenGL()
{
    // Lock texture to get raw pixel data
    uint8_t *pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(_texture, nullptr, (void **)&pixels, &pitch) != true) {
        return 0;
    }

    const SDL_PixelFormatDetails *tmp = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);

    for (int y = 0; y < _height; y++) {
        Uint32 *p = (Uint32 *)(pixels + pitch * y); // cast for a pointer increments by 4 bytes.(RGBA)
        for (int x = 0; x < _width; x++) {
            // *p = 0x00FF0000;
            *p = SDL_MapRGBA(tmp, nullptr, 255, 255, 255, 128);

            p++;
        }
    }

    if (!pixels) {
        spdlog::critical("shit");
    }

    spdlog::critical("{}", pitch);

    // Create an OpenGL texture
    GLuint texID;

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Set filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    SDL_UnlockTexture(_texture);
    return texID;
}

bool FrameEditor::init()
{
    _renderer = SDL_CreateRenderer(_window, nullptr);
    if (!_renderer) {
        return false;
    }

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, _width, _height);
    if (!_texture) {
        return false;
    }

    // Clear canvas to white
    if (!SDL_SetRenderTarget(_renderer, _texture)) {
        return false;
    }
    if (!SDL_SetRenderDrawColor(_renderer, 255, 255, 255, 255)) {
        return false;
    }
    if (!SDL_RenderClear(_renderer)) {
        return false;
    }
    if (!SDL_SetRenderTarget(_renderer, nullptr)) {
        return false;
    }

    static GLuint glTex = SDLTextureToOpenGL();

    // Set texture ID for ImGui
    _textureID = (ImTextureID)glTex;
    return true;
}

void FrameEditor::draw()
{
    SDL_SetRenderTarget(_renderer, _texture);
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255); // Black color for drawing

    for (size_t i = 1; i < points.size(); ++i) {
        SDL_RenderLine(_renderer, points[i - 1].x, points[i - 1].y, points[i].x, points[i].y);
    }

    SDL_SetRenderTarget(_renderer, nullptr);
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
    // draw();
}
