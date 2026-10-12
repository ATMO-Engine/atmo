#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>

namespace atmo
{
    namespace file_watcher
    {
        class Backend;
    } // namespace file_watcher

    /**
     * @brief Keeps an in-memory tree of the project directory in sync with the disk.
     *
     * A platform backend (inotify, FSEvents, ReadDirectoryChangesW) reports changed directories from its own thread. Those reports are queued and
     * applied to the tree on the main thread, once per frame, and a ReloadExplorerEvent is dispatched when the tree actually changed.
     * Every public function must be called from the main thread.
     */
    class FileWatcher
    {
    public:
        struct FileSystemNode {
            std::filesystem::path path;
            bool is_dir = false;

            std::vector<FileSystemNode> child;
        };

        static void InitFileSystem(const std::filesystem::path &root);

        /**
         * @brief Applies the changes reported by the backend to the tree.
         *
         * @return true if the tree changed.
         */
        static bool PollChanges();

        static const FileSystemNode &GetTree();

        static FileWatcher &Instance();

    private:
        struct PendingChange {
            std::filesystem::path dir;
            bool recursive = false;
        };

        FileWatcher();
        ~FileWatcher();

        void pushChange(const std::filesystem::path &dir, bool recursive);
        FileSystemNode *findNearestNode(const std::filesystem::path &dir);

        static FileSystemNode ScanFolder(const std::filesystem::path &path);
        static bool RefreshFolder(FileSystemNode &node);

        std::unique_ptr<file_watcher::Backend> m_backend;

        std::mutex m_pending_mutex;
        std::vector<PendingChange> m_pending;

        FileSystemNode m_root;
        uint32_t m_tick_listener = 0;
    };
} // namespace atmo
