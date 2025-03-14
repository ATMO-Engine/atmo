#ifndef SpriteEditor_HPP_
#define SpriteEditor_HPP_

#include <flecs.h>
#include <spdlog/spdlog.h>
#include "../../Widget/FileExplorer/FileExplorer.hpp"
#include "../../Widget/FrameEditor/FrameEditor.hpp"
#include "../../Widget/FrameTools/FrameTools.hpp"
#include "../Editor.hpp"

class SpriteEditor : public Editor
{
    public:
        SpriteEditor();
        void run() override;

    protected:
        const std::string editorName = "Scene Editor";

    private:
        FileExplorer fileExplorer;
        FrameEditor frameEditor;
        FrameTools frameTools;
};

#endif /* !SpriteEditor_HPP_ */
