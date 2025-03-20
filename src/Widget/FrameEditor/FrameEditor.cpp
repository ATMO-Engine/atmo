#include "FrameEditor.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "FrameEditor.hpp"

#include "spdlog/spdlog.h"


FrameEditor::FrameEditor(SDL_Renderer *renderer)
{
    _lastPos.x = -1;
    _lastPos.y = -1;
    _renderer = renderer;
}

FrameEditor::~FrameEditor()
{
    if (_texture) {
        SDL_DestroyTexture(_texture);
    }
}

void FrameEditor::clear()
{
    void* pixels;
    int pitch;
    SDL_LockTexture(_texture, nullptr, &pixels, &pitch);

    // Convert void* to Uint32* since we're working with 32-bit pixels
    Uint32* pixelData = (Uint32*)pixels;
    int pixelCount = (pitch / sizeof(Uint32)) * _heigth;

    // Set every pixel to white (RGBA: 255, 255, 255, 255)
    Uint32 white = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), nullptr, 0, 0, 0, 0);
    for (int i = 0; i < pixelCount; i++) {
        pixelData[i] = white;
    }
    SDL_UnlockTexture(_texture);
}

bool FrameEditor::init(int width, int heigth)
{
    const int GRID_SIZE = 50;
    _width = width;
    _heigth = heigth;
    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, _width, _heigth);
    if (!_texture) {
        return false;
    }
    _background = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, _width, _heigth);
    if (!_background) {
        return false;
    }

    void* pixels;
    int pitch;
    SDL_LockTexture(_background, nullptr, &pixels, &pitch);

    Uint32* pixelData = (Uint32*)pixels;
    Uint32 white = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), nullptr, 255, 255, 255, 255);
    Uint32 gray = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), nullptr, 200, 200, 200, 255);
    // Draw checkerboard pattern
    for (int y = 0; y < _heigth; y++) {
        for (int x = 0; x < _width; x++) {
            bool isWhite = ((x / GRID_SIZE) + (y / GRID_SIZE)) % 2 == 0;
            pixelData[y * _width + x] = isWhite ? white : gray;
        }
    }
    SDL_UnlockTexture(_background);



    SDL_LockTexture(_texture, nullptr, &pixels, &pitch);

    // Convert void* to Uint32* since we're working with 32-bit pixels
    pixelData = (Uint32*)pixels;
    int pixelCount = (pitch / sizeof(Uint32)) * _heigth;

    Uint32 trans = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), nullptr, 0, 0, 0, 0);
    // Set every pixel to white (RGBA: 255, 255, 255, 255)
    for (int i = 0; i < pixelCount; i++) {
        pixelData[i] = trans;
    }
    SDL_UnlockTexture(_texture);

    return true;
}

void FrameEditor::drawPoint(int x, int y)
{
    void* pixels;
    int pitch;
    SDL_LockTexture(_texture, nullptr, &pixels, &pitch);
    Uint32* pixelData = (Uint32*)pixels;

    Uint32 col = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), nullptr, _r, _g, _b, _a);
    int newX = 0;
    int newY = 0;
    for (int i = -1 * (_thickness / 2); i < _thickness / 2; i++) {
        newY = y - i;
        for (int j = -1 * (_thickness / 2); j < _thickness / 2; j++) {
            newX = x - j;
            if (newX < 0 || newX >= _width || newY < 0 || newY >= _heigth) {
                continue;
            }
            pixelData[newY * _width + newX] = col;
        }
    }

    SDL_UnlockTexture(_texture);
}

void FrameEditor::drawLine(int x, int y)
{
    int x0 = _lastPos.x;
    int y0 = _lastPos.y;
    // Calculate the differences in x and y direction
    int dx = abs(x - x0);  // Absolute difference in x (delta x)
    int dy = -abs(y - y0); // Absolute difference in y (negative delta y)

    // Determine the direction of movement
    int sx = (x0 < x) ? 1 : -1; // If x0 < x1, move right; otherwise, move left
    int sy = (y0 < y) ? 1 : -1; // If y0 < y1, move down; otherwise, move up

    // Initialize the error term (used to determine when to step in the y direction)
    int err = dx + dy; // Error value (sum of dx and dy, dy is negative)

    int e2; // Variable to store the double of error value

    // Loop until the line reaches its destination (x1, y1)
    while (true) {
        // Set the current pixel
        drawPoint(x0, y0);

        // If we have reached the endpoint, stop
        if (x0 == x && y0 == y) break;

        // Calculate 2 * err (to compare against dx and dy)
        e2 = 2 * err;

        // Adjust error and move in the x direction if necessary
        if (e2 >= dy) { // If error is large enough, move in the x direction
            err += dy; // Adjust error by dy
            x0 += sx;   // Move in the x direction (left or right)
        }

        // Adjust error and move in the y direction if necessary
        if (e2 <= dx) { // If error is small enough, move in the y direction
            err += dx; // Adjust error by dx
            y0 += sy;  // Move in the y direction (up or down)
        }
    }
}

void FrameEditor::draw()
{
    SDL_SetRenderTarget(_renderer, _texture);

    if (ImGui::IsMouseReleased(0)) {
        _lastPos.x = -1;
        _lastPos.y = -1;
    }
    if (ImGui::IsMouseDown(0)) {
        float x = 0;
        float y = 0;

        SDL_GetMouseState(&x, &y);
        float relativeX = x - _beginPos.x;
        float relativeY = y - _beginPos.y;

        if (relativeX < 0 || relativeX >= _width || relativeY < 0 || relativeY >= _heigth) {
            SDL_SetRenderTarget(_renderer, nullptr);
            return;
        }

        if (_lastPos.x == -1 && _lastPos.y == -1) {
            drawPoint(relativeX, relativeY);
        } else {
            drawLine(relativeX, relativeY);
        }
        _lastPos.x = relativeX;
        _lastPos.y = relativeY;

        SDL_UnlockTexture(_texture);
    }

    SDL_SetRenderTarget(_renderer, nullptr);
}

void FrameEditor::run()
{
    _beginPos = ImGui::GetCursorScreenPos();
    auto tmp = ImGui::GetCursorPos();

    draw();
    ImGui::Image((ImTextureID)(intptr_t)_background, ImVec2(_width, _heigth));
    ImGui::SetCursorPos(tmp);
    ImGui::Image((ImTextureID)(intptr_t)_texture, ImVec2(_width, _heigth));
}
