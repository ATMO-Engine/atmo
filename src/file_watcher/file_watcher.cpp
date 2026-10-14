#include "file_watcher.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/reload_explorer_event.hpp"
#include "core/event/events/progress_tick_event/progress_tick_event.hpp"
#include "core/thread/thread_pool.hpp"
#include "file_watcher_backend.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>

namespace fs = std::filesystem;

namespace atmo
{
#if !defined(__linux__) && !defined(__APPLE__) && !defined(_WIN32)
    std::unique_ptr<file_watcher::Backend> file_watcher::CreateBackend(const fs::path &, Backend::DirChangedCallback)
    {
        spdlog::warn("FileWatcher: no backend for this platform, the file explorer will not follow disk changes");
        return nullptr;
    }
#endif

    FileWatcher &FileWatcher::Instance()
    {
        static FileWatcher inst;
        return inst;
    }

    FileWatcher::FileWatcher()
    {
        // Backends may poll on the thread pool: construct it first so it is destroyed after the watcher.
        core::ThreadPool::Instance();
    }

    FileWatcher::~FileWatcher() = default;

    void FileWatcher::InitFileSystem(const fs::path &root)
    {
        auto &inst = Instance();

        inst.m_backend.reset();
        {
            std::lock_guard lock(inst.m_pending_mutex);
            inst.m_pending.clear();
        }
        inst.m_root = {};

        std::error_code ec;
        fs::path canonical_root = fs::weakly_canonical(root, ec);
        if (ec)
            canonical_root = root;

        if (!fs::is_directory(canonical_root, ec)) {
            spdlog::warn("FileWatcher: '{}' is not a directory, nothing watched", canonical_root.string());
            return;
        }

        // Start watching before scanning so changes made during the scan are not lost.
        inst.m_backend = file_watcher::CreateBackend(canonical_root, [&inst](const fs::path &dir, bool recursive) { inst.pushChange(dir, recursive); });
        inst.m_root = ScanFolder(canonical_root);

        if (inst.m_tick_listener == 0) {
            inst.m_tick_listener =
                core::event::EventRegistry::SetCallBack<core::event::events::ProgressTickEvent>([](core::event::events::ProgressTickEvent *) {
                    if (!PollChanges())
                        return;

                    auto reload = core::event::EventRegistry::Create<core::event::events::ReloadExplorerEvent>("Event::ReloadExplorerEvent");
                    core::event::EventRegistry::Dispatch(reload);
                });
        }

        auto reload = core::event::EventRegistry::Create<core::event::events::ReloadExplorerEvent>("Event::ReloadExplorerEvent");
        core::event::EventRegistry::Dispatch(reload);
    }

    bool FileWatcher::PollChanges()
    {
        auto &inst = Instance();

        std::vector<PendingChange> pending;
        {
            std::lock_guard lock(inst.m_pending_mutex);
            pending.swap(inst.m_pending);
        }

        if (pending.empty())
            return false;

        // Backends report the same directory many times for a burst of changes: refresh each one once.
        std::ranges::sort(pending, [](const PendingChange &a, const PendingChange &b) { return a.dir < b.dir; });
        std::vector<PendingChange> unique_changes;
        for (auto &change : pending) {
            if (!unique_changes.empty() && unique_changes.back().dir == change.dir)
                unique_changes.back().recursive |= change.recursive;
            else
                unique_changes.push_back(std::move(change));
        }

        bool changed = false;
        for (const auto &change : unique_changes) {
            FileSystemNode *node = inst.findNearestNode(change.dir);
            if (!node)
                continue;

            if (change.recursive) {
                *node = ScanFolder(node->path);
                changed = true;
            } else {
                changed |= RefreshFolder(*node);
            }
        }
        return changed;
    }

    const FileWatcher::FileSystemNode &FileWatcher::GetTree()
    {
        return Instance().m_root;
    }

    void FileWatcher::pushChange(const fs::path &dir, bool recursive)
    {
        std::lock_guard lock(m_pending_mutex);
        m_pending.push_back({ .dir = dir, .recursive = recursive });
    }

    FileWatcher::FileSystemNode *FileWatcher::findNearestNode(const fs::path &dir)
    {
        if (m_root.path.empty())
            return nullptr;

        fs::path relative = dir.lexically_relative(m_root.path);
        if (relative.empty() || *relative.begin() == "..")
            return nullptr;

        // Stop at the deepest directory already in the tree: refreshing it picks up any new subdirectories below.
        FileSystemNode *current = &m_root;
        for (const auto &part : relative) {
            if (part == ".")
                continue;

            auto it = std::ranges::find_if(current->child, [&part](const FileSystemNode &node) { return node.is_dir && node.path.filename() == part; });
            if (it == current->child.end())
                break;
            current = &(*it);
        }
        return current;
    }

    FileWatcher::FileSystemNode FileWatcher::ScanFolder(const fs::path &path)
    {
        FileSystemNode node{ .path = path, .is_dir = true, .child = {} };

        std::error_code ec;
        fs::directory_iterator it(path, fs::directory_options::skip_permission_denied, ec);
        if (ec) {
            spdlog::warn("FileWatcher: cannot scan '{}': {}", path.string(), ec.message());
            return node;
        }

        for (; !ec && it != fs::directory_iterator(); it.increment(ec)) {
            std::error_code entry_ec;
            if (it->is_directory(entry_ec) && !it->is_symlink(entry_ec))
                node.child.push_back(ScanFolder(it->path()));
            else
                node.child.push_back({ .path = it->path(), .is_dir = false, .child = {} });
        }
        return node;
    }

    bool FileWatcher::RefreshFolder(FileSystemNode &node)
    {
        std::error_code ec;
        fs::directory_iterator it(node.path, fs::directory_options::skip_permission_denied, ec);
        if (ec)
            return false; // The directory is gone: the refresh of its parent removes it.

        std::vector<FileSystemNode> children;
        std::size_t kept = 0;
        for (; !ec && it != fs::directory_iterator(); it.increment(ec)) {
            std::error_code entry_ec;
            const fs::path &path = it->path();
            bool is_dir = it->is_directory(entry_ec) && !it->is_symlink(entry_ec);

            auto existing = std::ranges::find_if(node.child, [&](const FileSystemNode &child) { return child.path == path && child.is_dir == is_dir; });
            if (existing != node.child.end()) {
                children.push_back(std::move(*existing));
                kept++;
            } else if (is_dir) {
                children.push_back(ScanFolder(path));
            } else {
                children.push_back({ .path = path, .is_dir = false, .child = {} });
            }
        }

        bool changed = kept != node.child.size() || kept != children.size();
        node.child = std::move(children);
        return changed;
    }
} // namespace atmo
