#ifndef FileExplorer_HPP_
#define FileExplorer_HPP_

#include "../Widget.hpp"

class FileExplorer : public Widget {
    public:
        FileExplorer();
        ~FileExplorer();

        void run() override;

    protected:
        const std::string editorName = "Scene Editor";
};

#endif /* !FileExplorer_HPP_ */
