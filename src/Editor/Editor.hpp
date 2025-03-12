#ifndef EDITOR_HPP_
#define EDITOR_HPP_

#include <string>

class Editor
{
    public:
        virtual void run() = 0;
        std::string getEditorName() { return editorName; }

    protected:
        const std::string editorName;
};

#endif /* !EDITOR_HPP_ */
