#ifndef SCENEEDITOR_HPP_
#define SCENEEDITOR_HPP_

#include "../Editor.hpp"

class SceneEditor : public Editor {
    public:
        SceneEditor();
        ~SceneEditor();

        void run() override;

    protected:
        const std::string editorName = "Scene Editor";
};

#endif /* !SCENEEDITOR_HPP_ */
