#pragma once

#include <filesystem>
#include <sys/inotify.h>
#include <vector>
#include "editor/editor_entities/ui_file_explorer/ui_dir_node/ui_dir_node.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_node/ui_file_node.hpp"
#ifdef __linux__

#include <mutex>
#include <string>
#include <unordered_map>

namespace atmo
{
    class FileWatcher
    {
    public:
        struct FileSystemNode {
            std::filesystem::path path;
            bool is_dir;
            int wd;

            std::vector<FileSystemNode> child;
        };

        struct FileChange {
            struct PendingChange {
                int wd;
                uint32_t mask;
                uint32_t cookie;
                std::string file_name;
            };

            PendingChange file_context;
            std::filesystem::path wd_path;
        };

        static bool IsValid() noexcept
        {
            return Instance().m_inotify_fd >= 0;
        }

        static void AddWatch(const std::filesystem::path &path);
        static void RemoveWatch(const std::filesystem::path &path);
        static void ClearWatches();

        static void InitFileSystem(const std::filesystem::path &root);

        static std::shared_ptr<atmo::core::ecs::entities::UIFileExplorerDirNode>
        GetFileSystemFoldableTree(const std::vector<std::string> &open_paths, const std::filesystem::path &focused_path);

        static FileWatcher &Instance();

    private:
        FileWatcher();
        ~FileWatcher();

        std::shared_ptr<atmo::core::ecs::entities::UIFileExplorerFileNode>
        createFoldableTreeLeaf(const FileSystemNode &file, const std::filesystem::path &focused_path);
        std::shared_ptr<atmo::core::ecs::entities::UIFileExplorerDirNode>
        createFoldableTree(const FileSystemNode &file, const std::vector<std::string> &open_paths, const std::filesystem::path &focused_path);

        FileSystemNode *getWorkingNode(const FileSystemNode *root, const std::filesystem::path &changes);

        void createAction(const inotify_event *evt, const FileChange &changes);
        void deleteAction(const inotify_event *evt, const FileChange &changes);
        void moveInAction(const inotify_event *evt, const FileChange &changes);
        void moveOutAction(const inotify_event *evt, const FileChange &changes);

        void workerLoop(); // tourne dans le ThreadPool, bloque sur read()

        static FileSystemNode ScanFolder(const std::filesystem::path &path, bool recursive = true);

        int m_inotify_fd = -1;

        std::mutex m_watchMutex;
        std::mutex m_graphMutex;
        // map bidirectionnelle wd <-> path pour retrouver le path depuis un événement
        std::unordered_map<int, std::filesystem::path> m_wd_to_path;
        std::unordered_map<std::filesystem::path, int> m_path_to_wd;
        FileSystemNode m_root;
    };
} // namespace atmo

#endif // __linux__
