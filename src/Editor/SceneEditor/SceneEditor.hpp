#ifndef SCENEEDITOR_HPP_
#define SCENEEDITOR_HPP_

#include <flecs.h>
#include "../Editor.hpp"
#include "../../Widget/SceneHierarchy/SceneHierarchy.hpp"
#include "../../Widget/FileExplorer/FileExplorer.hpp"

class SceneEditor : public Editor
{
    public:
        SceneEditor();
        void run() override;

    protected:
        const std::string editorName = "Scene Editor";

    private:
        flecs::world world;
        SceneHierarchy sceneHierarchy;
        FileExplorer fileExplorer;
};

#endif /* !SCENEEDITOR_HPP_ */
