#ifndef SCENEEDITOR_HPP_
#define SCENEEDITOR_HPP_

#include <flecs.h>
#include <spdlog/spdlog.h>
#include "../../Widget/EntityInspector/EntityInspector.hpp"
#include "../../Widget/FileExplorer/FileExplorer.hpp"
#include "../../Widget/SceneHierarchy/SceneHierarchy.hpp"
#include "../../Widget/Console/Console.hpp"
#include "../../Widget/SceneView/SceneView.hpp"
#include "../Editor.hpp"
#include "SDL3/SDL_video.h"

class SceneEditor : public Editor
{
    public:
        SceneEditor();
        void run() override;
        void init(SDL_Window *window);

        std::string *getRenameBuffer() { return sceneHierarchy.getRenameBuffer(); }
        flecs::world &getEcs() { return ecs; }
        flecs::entity_t getSelectedEntity() const { return selectedEntity; }

    protected:
        const std::string editorName = "Scene Editor";

    private:
        flecs::world ecs;
        flecs::entity_t selectedEntity;
        SceneHierarchy sceneHierarchy;
        FileExplorer fileExplorer;
        EntityInspector entityInspector;
        Console console;
        SceneView _sceneView;
};

#endif /* !SCENEEDITOR_HPP_ */
