#include <filesystem>
#include "imgui.h"
#include "spdlog/spdlog.h"

#include "FileExplorer.hpp"

FileExplorer::FileExplorer(std::filesystem::path dirPath) { _actualPath = dirPath; }

FileExplorer::~FileExplorer() {}

void FileExplorer::run()
{
    ImGui::SameLine();
    if (ImGui::Button("refresh")) {
        refresh();
    }
    display(0, _fileList);

}

void FileExplorer::display(int depth, std::vector<File> &list)
{
    for (auto &file : list) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * 20.0f); // Adjust 20.0f to your desired indentation
        if (file.getType() == 'd') {
            if (ImGui::CollapsingHeader(file.getPath().filename().c_str())) {
                // Content inside the extendable box
                display(depth + 1, file.getSubDirContent());
            }
        }
        else {
            ImGui::Text("%s", file.getPath().filename().string().c_str());
        }
    }
}

void FileExplorer::refresh() { _fileList = listSubDir(_actualPath); }

std::vector<FileExplorer::File> FileExplorer::listSubDir(const std::filesystem::path &wd)
{
    std::vector<File> directories;
    std::vector<File> files;

    for (const auto &elm : std::filesystem::directory_iterator(wd)) {
        if (elm.is_directory()) {
            directories.push_back(File(elm.path(), 'd', listSubDir(elm.path())));
        }
        else {
            files.push_back(File(elm.path(), 'f', {}));
        }
    }

    for (const auto &file : files) {
        directories.push_back(file);
    }
    return directories;
}

FileExplorer::File::File(std::filesystem::path path, char type, std::vector<File> content)
{
    _path = path;
    _type = type;
    _subDirContent = content;
}
