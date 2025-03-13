#ifndef SCENEEDITOR_HPP_
#define SCENEEDITOR_HPP_

#include <flecs.h>
#include "../Editor.hpp"

class SceneEditor : public Editor
{
    public:
        SceneEditor();
        void run() override;

    protected:
        const std::string editorName = "Scene Editor";
        flecs::world world;
};

#endif /* !SCENEEDITOR_HPP_ */
