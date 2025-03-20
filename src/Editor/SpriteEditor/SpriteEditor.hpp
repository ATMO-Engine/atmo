#ifndef SpriteEditor_HPP_
#define SpriteEditor_HPP_

#include <flecs.h>
#include <spdlog/spdlog.h>
#include "../../Widget/ColorPicker/ColorPicker.hpp"
#include "../../Widget/FileExplorer/FileExplorer.hpp"
#include "../../Widget/FrameEditor/FrameEditor.hpp"
#include "../Editor.hpp"
#include "SDL3/SDL_render.h"

class SpriteEditor : public Editor
{
    public:
        SpriteEditor(SDL_Renderer *renderer);
        void run() override;
        void init();

    protected:
        const std::string editorName = "Scene Editor";

    private:
        FileExplorer fileExplorer;
        FrameEditor frameEditor;
        ColorPicker colorPicker;
        float _currentColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        ColorPicker::Tool _selectedTool;
};

#endif /* !SpriteEditor_HPP_ */
