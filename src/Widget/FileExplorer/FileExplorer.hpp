#ifndef FileExplorer_HPP_
#define FileExplorer_HPP_

#include <filesystem>
#include <vector>
#include "../Widget.hpp"


class FileExplorer : public Widget
{
    public:
        class File
        {
            public:
                File(std::filesystem::path path, char type, std::vector<File> content);
                ~File() = default;

                std::filesystem::path getPath() const { return _path; };
                char getType() const { return _type; };
                std::vector<File> &getSubDirContent() { return _subDirContent; };

            private:
                std::filesystem::path _path;
                char _type;
                std::vector<File> _subDirContent;
        };
        FileExplorer(std::filesystem::path dirPath);
        ~FileExplorer();

        void run() override;
        void refresh();

    protected:
        void display(int depth, std::vector<File> &list);
        const std::string widgetName = "File Explorer";
        std::filesystem::path _actualPath;

    private:
        std::vector<File> listSubDir(const std::filesystem::path &wd);

        std::vector<File> _fileList;
};

#endif /* !FileExplorer_HPP_ */
