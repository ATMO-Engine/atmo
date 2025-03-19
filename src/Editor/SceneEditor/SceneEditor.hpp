#ifndef SCENEEDITOR_HPP_
#define SCENEEDITOR_HPP_

#include <flecs.h>
#include <spdlog/spdlog.h>
#include "../../Widget/EntityInspector/EntityInspector.hpp"
#include "../../Widget/FileExplorer/FileExplorer.hpp"
#include "../../Widget/SceneHierarchy/SceneHierarchy.hpp"
#include "../../Widget/SceneView/SceneView.hpp"
#include "../Editor.hpp"
#include "SDL3/SDL_video.h"

class SceneEditor : public Editor
{
    public:
        SceneEditor();
        void run() override;
        void init(SDL_Window *window);

    protected:
        const std::string editorName = "Scene Editor";

    private:
        flecs::world ecs;
        flecs::entity_t selectedEntity;
        SceneHierarchy sceneHierarchy;
        FileExplorer fileExplorer;
        EntityInspector entityInspector;
        SceneView _sceneView;
};

#endif /* !SCENEEDITOR_HPP_ */
