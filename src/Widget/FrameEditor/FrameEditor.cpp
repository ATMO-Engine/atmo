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

bool FrameEditor::init()
{
    return true;
}

void FrameEditor::draw()
{
}

void FrameEditor::run()
{
}
