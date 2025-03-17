#ifndef SCENEEDITOR_HPP_
#define SCENEEDITOR_HPP_

#include <flecs.h>
#include <spdlog/spdlog.h>
#include "../../Widget/EntityInspector/EntityInspector.hpp"
#include "../../Widget/FileExplorer/FileExplorer.hpp"
#include "../../Widget/SceneHierarchy/SceneHierarchy.hpp"
#include "../Editor.hpp"

class SceneEditor : public Editor
{
    public:
        SceneEditor();
        void run() override;

    protected:
        const std::string editorName = "Scene Editor";

    private:
        flecs::world ecs;
        flecs::entity_t selectedEntity;
        SceneHierarchy sceneHierarchy;
        FileExplorer fileExplorer;
        EntityInspector entityInspector;
};

#endif /* !SCENEEDITOR_HPP_ */
