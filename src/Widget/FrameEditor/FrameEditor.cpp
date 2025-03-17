
#include "SDL3/SDL_rect.h"
#include "spdlog/spdlog.h"

#include "FrameEditor.hpp"

FrameEditor::FrameEditor(SDL_Window *window)
{
    _window = window;
}

FrameEditor::~FrameEditor()
{
    SDL_DestroyRenderer(_renderer);
    SDL_DestroyTexture(_texture);
}


GLuint FrameEditor::SDLTextureToOpenGL() {
    float width, height;
    SDL_GetTextureSize(_texture, &width, &height);
    spdlog::warn("{} {}", width, height);

    spdlog::info("est cuit");
    // Lock texture to get raw pixel data

    SDL_Rect *shit = new SDL_Rect(_width, _height);
    spdlog::info("vous me le branlez");
    void* pixels;
    int pitch;
    if (SDL_LockTexture(_texture, shit, &pixels, &pitch) != 0) {
        return 0;
    }

    if (!pixels) {
        spdlog::critical("shit");
    }

    spdlog::critical("{}", pitch);

    spdlog::info("celui la");
    // Create an OpenGL texture
    GLuint texID;

    spdlog::info("shit 0");
    glGenTextures(1, &texID);
    spdlog::info("shit 1");
    glBindTexture(GL_TEXTURE_2D, texID);
    spdlog::info("shit 2");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    spdlog::info("shit");
    // Set filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    spdlog::info("shit");
    SDL_UnlockTexture(_texture);

    spdlog::info("shit");
    return texID;
}

bool FrameEditor::init()
{
    _renderer = SDL_CreateRenderer(_window, nullptr);
    if (!_renderer) {
        return false;
    }

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_TARGET, _width, _height);
    if (!_texture) {
        return false;
    }

    // Clear canvas to white
    if (!SDL_SetRenderTarget(_renderer, _texture)) {
        spdlog::info("shit 1");
    }
    if (!SDL_SetRenderDrawColor(_renderer, 255, 255, 255, 255)) {
        spdlog::info("shit 2");
    }
    if (!SDL_RenderClear(_renderer)) {
        spdlog::info("shit 3");
    }
    if (!SDL_SetRenderTarget(_renderer, nullptr)) {
        spdlog::info("shit 4");
    }


    spdlog::info("le caca");
    static GLuint caca = SDLTextureToOpenGL();
    spdlog::info("le caca est cuit");

    // Set texture ID for ImGui
    _textureID = (ImTextureID)caca;
    return true;
}

void FrameEditor::draw()
{
    SDL_SetRenderTarget(_renderer, _texture);
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);  // Black color for drawing

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
    ImGui::Image((intptr_t)_textureID, canvasSize);

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
