#ifndef SCENEEDITOR_HPP_
#define SCENEEDITOR_HPP_

#include <flecs.h>
#include <spdlog/spdlog.h>
#include "../Editor.hpp"
#include "../../Widget/SceneHierarchy/SceneHierarchy.hpp"

class SceneEditor : public Editor
{
    public:
        SceneEditor();
        void run() override;

    protected:
        const std::string editorName = "Scene Editor";

    private:
        flecs::world ecs;
        SceneHierarchy sceneHierarchy;
};

#endif /* !SCENEEDITOR_HPP_ */
