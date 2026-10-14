#pragma once

#include <filesystem>
#include <functional>
#include <memory>

namespace atmo::file_watcher
{
    /**
     * @brief Platform specific watcher of a directory tree.
     *
     * Implementations report, from any thread, the directories whose entries changed. When events were lost (queue overflow, dropped events),
     * they report a directory with recursive set, meaning its whole subtree must be rescanned.
     */
    class Backend
    {
    public:
        using DirChangedCallback = std::function<void(const std::filesystem::path &dir, bool recursive)>;

        virtual ~Backend() = default;
    };

    /**
     * @brief Starts watching root recursively. Defined once per platform (file_watcher_inotify.cpp, file_watcher_fsevents.cpp,
     * file_watcher_win32.cpp).
     *
     * @return nullptr if the platform is unsupported or the watch could not be started.
     */
    std::unique_ptr<Backend> CreateBackend(const std::filesystem::path &root, Backend::DirChangedCallback on_dir_changed);
} // namespace atmo::file_watcher
